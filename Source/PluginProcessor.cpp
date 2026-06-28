#include "PluginProcessor.h"
#include "PluginEditor.h"

#include <cmath>

using namespace nscm;

namespace
{
float getChoice (juce::AudioProcessorValueTreeState& p, const char* id, int fallback)
{
    if (auto* c = dynamic_cast<juce::AudioParameterChoice*> (p.getParameter (id)))
        return (float) c->getIndex();
    return (float) fallback;
}

float getValue (juce::AudioProcessorValueTreeState& p, const char* id)
{
    if (auto* v = p.getRawParameterValue (id))
        return v->load();
    return 0.0f;
}
} // namespace

NSColourMapAudioProcessor::NSColourMapAudioProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      parameters (*this, nullptr, "Parameters", nscm::params::createParameterLayout())
{
}

int NSColourMapAudioProcessor::qualityToMaxVoices (int quality) const noexcept
{
    return quality >= 1 ? 28 : 20; // STFT tiers : 0 Latency
}

void NSColourMapAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate > 0.0 ? sampleRate : 44100.0;
    const int numCh = juce::jmax (getTotalNumInputChannels(), getTotalNumOutputChannels(), 2);
    const int block = juce::jmax (16, samplesPerBlock);

    affectedRange.prepare (currentSampleRate, block, numCh);
    transientDetector.prepare (currentSampleRate);
    colourCore.prepare (currentSampleRate);
    formantTone.prepare (currentSampleRate);
    limiter.prepare (currentSampleRate);
    for (int i = 0; i < 3; ++i)
        spectrals[(size_t) i].prepare (currentSampleRate, 9 + i, numCh); // 512 / 1024 / 2048-pt STFT tiers
    analyzer.prepare (currentSampleRate, 11);          // EQ-style spectrum display
    chroma.prepare (currentSampleRate, 11);            // Audio-mode note detection
    chordState.reset();

    juce::dsp::ProcessSpec spec;
    spec.sampleRate = currentSampleRate;
    spec.maximumBlockSize = (juce::uint32) block;
    spec.numChannels = (juce::uint32) (numCh * 2); // 0..nc-1 = dry, nc.. = original active
    dryDelay.prepare (spec);
    dryDelay.setMaximumDelayInSamples (spectrals[2].getLatency() + 16);
    dryDelay.reset();

    dryBuf.setSize (numCh, block, false, false, true);
    activeBuf.setSize (numCh, block, false, false, true);
    snapBuf.setSize (numCh, block, false, false, true);
    tunedBuf.setSize (numCh, block, false, false, true);
    transientEnv.assign ((size_t) block, 0.0f);

    const double ramp = 0.03;
    for (auto* s : { &colorSm, &amountSm, &melodySm, &mixSm, &outputSm, &gammaSm, &gateSm })
        s->reset (currentSampleRate, ramp);
    formantSm.reset (currentSampleRate, 0.06);
    colourGain.reset (currentSampleRate, 0.08);          // ~80 ms colour fade in/out
    colourGain.setCurrentAndTargetValue (0.0f);
    dcX1.fill (0.0f); dcY1.fill (0.0f);

    colorSm.setCurrentAndTargetValue   (getValue (parameters, params::color));
    amountSm.setCurrentAndTargetValue  (getValue (parameters, params::amount));
    melodySm.setCurrentAndTargetValue  (getValue (parameters, params::melody));
    mixSm.setCurrentAndTargetValue     (getValue (parameters, params::mix));
    outputSm.setCurrentAndTargetValue  (juce::Decibels::decibelsToGain (getValue (parameters, params::output)));
    formantSm.setCurrentAndTargetValue (getValue (parameters, params::formant));
    gammaSm.setCurrentAndTargetValue   (getValue (parameters, params::gamma));
    gateSm.setCurrentAndTargetValue    (getValue (parameters, params::gate));

    // Report the correct latency for the CURRENT quality up front, so offline
    // render / PDC reads the right value (it queries latency after prepareToPlay,
    // before the first processBlock). 0 Latency = 0; High Quality = STFT fftSize.
    const int q = (int) getChoice (parameters, params::quality, 0);
    reportedLatency = q >= 1 ? spectrals[(size_t) juce::jlimit (0, 2, q - 1)].getLatency() : 0;
    dryDelay.setDelay ((float) reportedLatency);
    setLatencySamples (reportedLatency);
}

void NSColourMapAudioProcessor::releaseResources() {}

bool NSColourMapAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto& in  = layouts.getMainInputChannelSet();
    const auto& out = layouts.getMainOutputChannelSet();
    return in == out && (out == juce::AudioChannelSet::mono() || out == juce::AudioChannelSet::stereo());
}

void NSColourMapAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;

    const int numCh      = juce::jmin (buffer.getNumChannels(), 2);
    const int numSamples = buffer.getNumSamples();

    for (int ch = getTotalNumInputChannels(); ch < getTotalNumOutputChannels(); ++ch)
        buffer.clear (ch, 0, numSamples);

    if (numCh <= 0 || numSamples <= 0)
        return;

    // ── Input safety: DC blocker (spec §12.1) ─────────────────────────────────
    for (int ch = 0; ch < numCh; ++ch)
    {
        float* d = buffer.getWritePointer (ch);
        float x1 = dcX1[(size_t) ch], y1 = dcY1[(size_t) ch];
        for (int s = 0; s < numSamples; ++s)
        {
            const float x = d[s];
            const float y = x - x1 + 0.9975f * y1;  // ~one-pole highpass ≈ 20 Hz
            x1 = x; y1 = y;
            d[s] = y;
        }
        dcX1[(size_t) ch] = x1; dcY1[(size_t) ch] = y1;
    }

    // ── Parameters ────────────────────────────────────────────────────────────
    const int   modeIdx   = (int) getChoice (parameters, params::mode, 0);
    const auto  character = (Character) (int) getChoice (parameters, params::character, 1);
    const auto& profile   = getCharacterProfile (character);
    const bool  freeze    = getValue (parameters, params::midiFreeze) > 0.5f;
    const int   keyIdx    = (int) getChoice (parameters, params::key, 0);
    const auto  scaleType = (Scale) (int) getChoice (parameters, params::scale, 7);
    const int   shift     = (int) std::lround (getValue (parameters, params::scaleShift));
    const int   qualityIx = (int) getChoice (parameters, params::quality, 0);
    const int   maxVoices = qualityToMaxVoices (qualityIx);
    const float lowCutHz  = getValue (parameters, params::lowCut);
    const float highCutHz = getValue (parameters, params::highCut);
    const float transAmt  = juce::jlimit (0.0f, 1.5f, getValue (parameters, params::transient) + profile.transientBias);
    const bool  sideMute  = getValue (parameters, params::sideMute) > 0.5f;

    // Quality slider: tier 0 = 0 Latency (oscillator core); tiers 1..3 = STFT snap
    // at 512/1024/2048 (more quality + more latency). Update reported latency on change.
    const bool engineHQ   = (qualityIx >= 1);
    const int  tierIx     = juce::jlimit (0, 2, qualityIx - 1);
    const int  wantLatency = engineHQ ? spectrals[(size_t) tierIx].getLatency() : 0;
    if (wantLatency != reportedLatency)
    {
        reportedLatency = wantLatency;
        setLatencySamples (wantLatency);
        dryDelay.setDelay ((float) wantLatency);
    }

    colorSm.setTargetValue   (getValue (parameters, params::color));
    amountSm.setTargetValue  (getValue (parameters, params::amount));
    melodySm.setTargetValue  (getValue (parameters, params::melody));
    mixSm.setTargetValue     (getValue (parameters, params::mix));
    outputSm.setTargetValue  (juce::Decibels::decibelsToGain (getValue (parameters, params::output)));
    formantSm.setTargetValue (getValue (parameters, params::formant));
    gammaSm.setTargetValue   (getValue (parameters, params::gamma));
    gateSm.setTargetValue    (getValue (parameters, params::gate));

    // ── MIDI ──────────────────────────────────────────────────────────────────
    bool sawNote = false;
    for (const auto meta : midi)
    {
        const auto msg = meta.getMessage();
        if (msg.isNoteOn())  { chordState.noteOn (msg.getNoteNumber()); sawNote = true; }
        else if (msg.isNoteOff()) chordState.noteOff (msg.getNoteNumber());
        else if (msg.isAllNotesOff() || msg.isAllSoundOff()) chordState.allNotesOff();
    }
    if (sawNote)
        uiMidiActivity.store (1.0f);

    // ── Pitch grid ────────────────────────────────────────────────────────────
    const juce::uint16 scaleMask = makeScaleNoteSet (keyIdx, scaleType).mask;
    const juce::uint16 midiMask  = chordState.getActiveMask (freeze);
    juce::uint16 mask = scaleMask;
    if (modeIdx == 1) mask = midiMask;                          // MIDI
    else if (modeIdx == 2) mask = (juce::uint16) (scaleMask | midiMask); // Hybrid
    else if (modeIdx == 3)                                      // UI: clicked notes
    {
        const auto cm = (juce::uint16) uiCustomMask.load();
        mask = cm != 0 ? cm : scaleMask;                       // fall back to scale until notes are picked
    }
    else if (modeIdx == 4)                                      // Audio: notes detected from the input
    {
        const float* inp[2];
        for (int ch = 0; ch < numCh; ++ch) inp[ch] = buffer.getReadPointer (ch);
        chroma.push (inp, numCh, numSamples);
        const auto cm = chroma.getMask();
        mask = cm != 0 ? cm : scaleMask;                       // fall back to scale when input is silent
    }
    // Scale (0) uses the scale mask.

    // The grid is "active" when there are target notes. When MIDI notes are
    // released (and Freeze is off) the mask becomes 0; we keep the last targets
    // for the fade and let colourGain ramp to 0 so the colour stops smoothly.
    const bool hasGrid = (mask != 0);
    if (hasGrid)
        fillTargetNotes (targets, mask, shift, maxVoices);
    colourGain.setTargetValue (hasGrid ? 1.0f : 0.0f);
    colourCore.setTargets (targets, maxVoices);

    uiHeldMask.store (midiMask);
    uiTargetMask.store (hasGrid ? mask : 0);
    uiVoiceCount.store (hasGrid ? colourCore.getActiveVoiceCount() : 0);

    // ── Idle: nothing to colour and the fade has finished -> pass dry through ──
    if (! hasGrid && colourGain.getCurrentValue() < 5.0e-4f)
    {
        colourGain.setCurrentAndTargetValue (0.0f);
        for (int s = 0; s < numSamples; ++s)
        {
            const float g = outputSm.getNextValue();
            for (int ch = 0; ch < numCh; ++ch)
            {
                dryDelay.pushSample (ch, buffer.getSample (ch, s));      // keep delay aligned
                buffer.setSample (ch, s, dryDelay.popSample (ch) * g);
            }
        }
        float* out[2];
        for (int ch = 0; ch < numCh; ++ch) out[ch] = buffer.getWritePointer (ch);
        limiter.process (out, numCh, numSamples);
        analyzer.pushBlock (out, numCh, numSamples);

        // keep block-rate smoothers moving so they are correct when we resume
        colorSm.skip (numSamples); amountSm.skip (numSamples); melodySm.skip (numSamples); gateSm.skip (numSamples);
        formantSm.skip (numSamples); gammaSm.skip (numSamples); mixSm.skip (numSamples);
        uiInputEnergy.store (0.0f); uiTunedEnergy.store (0.0f); uiColoredEnergy.store (0.0f);
        uiTransientFlash.store (0.0f);
        uiMidiActivity.store (juce::jmax (0.0f, uiMidiActivity.load() - 0.03f));
        return;
    }

    // ── Affected range split ──────────────────────────────────────────────────
    affectedRange.setRange (lowCutHz, highCutHz);
    uiLowHz.store (affectedRange.getLowHz());
    uiHighHz.store (affectedRange.getHighHz());

    float* dry[2]; float* active[2]; float* snap[2]; float* tuned[2];
    const float* in[2];
    for (int ch = 0; ch < numCh; ++ch)
    {
        dry[ch]    = dryBuf.getWritePointer (ch);
        active[ch] = activeBuf.getWritePointer (ch);
        snap[ch]   = snapBuf.getWritePointer (ch);
        tuned[ch]  = tunedBuf.getWritePointer (ch);
        in[ch]     = buffer.getReadPointer (ch);
        std::copy (in[ch], in[ch] + numSamples, dry[ch]);
    }

    affectedRange.process (dry, active, numCh, numSamples);     // active = original in-range band

    // snap = the colour source. In HQ it is the STFT pitch-class snap of the active
    // band; in 0-Latency it is just the active band.
    for (int ch = 0; ch < numCh; ++ch)
        std::copy (active[ch], active[ch] + numSamples, snap[ch]);

    if (engineHQ)
    {
        auto& sp = spectrals[(size_t) tierIx];
        sp.setGrid ((juce::uint16) mask);
        sp.setStrength (juce::jlimit (0.3f, 1.0f,
                        0.3f + 0.7f * amountSm.getCurrentValue() * juce::jmin (colorSm.getCurrentValue(), 1.0f)));
        sp.process (snap, numCh, numSamples);                  // snapped + delayed by fftSize

        // Delay dry and the ORIGINAL active by the same fftSize so recombination aligns.
        for (int ch = 0; ch < numCh; ++ch)
            for (int s = 0; s < numSamples; ++s)
            {
                dryDelay.pushSample (ch,          dry[ch][s]);
                dryDelay.pushSample (numCh + ch,  active[ch][s]);
                dry[ch][s]    = dryDelay.popSample (ch);
                active[ch][s] = dryDelay.popSample (numCh + ch);
            }
    }

    for (int ch = 0; ch < numCh; ++ch)
        std::copy (snap[ch], snap[ch] + numSamples, tuned[ch]); // core colours the snapped band

    // Transient envelope on the (snapped) colour-source band.
    float flash = 0.0f;
    transientDetector.process (snap, numCh, numSamples, transientEnv.data(), flash);
    uiTransientFlash.store (flash);

    // ── Colour mapping core ───────────────────────────────────────────────────
    // Advance the block-rate smoothers (skip = step by numSamples and return the
    // new value) so COLOR / Amount / Gate actually track the knobs.
    const float colorNow = colorSm.skip (numSamples);
    ColourMappingCore::Settings cset;
    cset.color01    = juce::jmin (colorNow, 1.0f);
    cset.colorBoost = juce::jlimit (0.0f, 1.0f, colorNow - 1.0f);
    cset.amount     = amountSm.skip (numSamples);
    cset.melody     = melodySm.skip (numSamples);
    cset.gate       = gateSm.skip (numSamples);
    cset.morph      = getValue (parameters, params::morph);
    cset.sizzle     = getValue (parameters, params::air);
    cset.profile    = profile;

    ColourMappingCore::Energies energies;
    colourCore.process (tuned, snap, numCh, numSamples, cset, energies); // baseline = snapped band

    uiInputEnergy.store (energies.input);
    uiTunedEnergy.store (energies.tuned);
    uiColoredEnergy.store (energies.colored);

    // ── Pseudo formant / gamma (real vowel formants, exaggerated by Gamma) ─────
    const float formantSt = formantSm.skip (numSamples);
    const float gammaNow  = gammaSm.skip (numSamples);
    const float baseAmt   = juce::jlimit (0.0f, 1.0f,
                                          std::abs (formantSt) / 24.0f * 0.6f + profile.formantReact * 0.15f);
    formantTone.update (formantSt, baseAmt, gammaNow, numSamples);
    formantTone.process (tuned, numCh, numSamples);

    // ── Side mute (collapse processed band toward mono) ───────────────────────
    if (sideMute && numCh >= 2)
    {
        for (int s = 0; s < numSamples; ++s)
        {
            const float mid = 0.5f * (tuned[0][s] + tuned[1][s]);
            tuned[0][s] = mid;
            tuned[1][s] = mid;
        }
    }

    // ── Combine: transient restore, mix, output ───────────────────────────────
    for (int s = 0; s < numSamples; ++s)
    {
        const float mix   = mixSm.getNextValue();
        const float outGn = outputSm.getNextValue();
        const float cg    = colourGain.getNextValue();                   // fades the colour out on note-off
        const float tg    = juce::jlimit (0.0f, 1.0f, transientEnv[(size_t) s] * transAmt);

        for (int ch = 0; ch < numCh; ++ch)
        {
            const float act  = active[ch][s];
            const float proc = tuned[ch][s] + tg * (act - tuned[ch][s]); // attacks pass dry
            float outSample  = dry[ch][s] + mix * cg * (proc - act);     // out-of-range stays dry
            outSample *= outGn;
            buffer.setSample (ch, s, outSample);
        }
    }

    float* out[2];
    for (int ch = 0; ch < numCh; ++ch)
        out[ch] = buffer.getWritePointer (ch);
    limiter.process (out, numCh, numSamples);
    analyzer.pushBlock (out, numCh, numSamples);

    uiMidiActivity.store (juce::jmax (0.0f, uiMidiActivity.load() - 0.03f));
}

juce::AudioProcessorEditor* NSColourMapAudioProcessor::createEditor()
{
    return new NSColourMapAudioProcessorEditor (*this);
}

void NSColourMapAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = parameters.copyState().createXml())
        copyXmlToBinary (*xml, destData);
}

void NSColourMapAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (parameters.state.getType()))
        {
            parameters.replaceState (juce::ValueTree::fromXml (*xml));
            uiCustomMask.store ((juce::uint32) (int) parameters.state.getProperty ("customGrid", 0) & 0xFFFu);
        }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new NSColourMapAudioProcessor();
}
