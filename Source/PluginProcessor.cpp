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
    return quality == 1 ? 28 : 20; // High Quality : 0 Latency
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
    chordState.reset();

    dryBuf.setSize (numCh, block, false, false, true);
    activeBuf.setSize (numCh, block, false, false, true);
    tunedBuf.setSize (numCh, block, false, false, true);
    transientEnv.assign ((size_t) block, 0.0f);

    const double ramp = 0.03;
    for (auto* s : { &colorSm, &amountSm, &mixSm, &outputSm, &gammaSm, &gateSm })
        s->reset (currentSampleRate, ramp);
    formantSm.reset (currentSampleRate, 0.06);

    colorSm.setCurrentAndTargetValue   (getValue (parameters, params::color));
    amountSm.setCurrentAndTargetValue  (getValue (parameters, params::amount));
    mixSm.setCurrentAndTargetValue     (getValue (parameters, params::mix));
    outputSm.setCurrentAndTargetValue  (juce::Decibels::decibelsToGain (getValue (parameters, params::output)));
    formantSm.setCurrentAndTargetValue (getValue (parameters, params::formant));
    gammaSm.setCurrentAndTargetValue   (getValue (parameters, params::gamma));
    gateSm.setCurrentAndTargetValue    (getValue (parameters, params::gate));

    setLatencySamples (0);
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

    // ── Parameters ────────────────────────────────────────────────────────────
    const int   modeIdx   = (int) getChoice (parameters, params::mode, 0);
    const auto  character = (Character) (int) getChoice (parameters, params::character, 1);
    const auto& profile   = getCharacterProfile (character);
    const bool  freeze    = getValue (parameters, params::midiFreeze) > 0.5f;
    const int   keyIdx    = (int) getChoice (parameters, params::key, 0);
    const auto  scaleType = (Scale) (int) getChoice (parameters, params::scale, 7);
    const int   shift     = (int) std::lround (getValue (parameters, params::scaleShift));
    const int   maxVoices = qualityToMaxVoices ((int) getChoice (parameters, params::quality, 0));
    const float lowCutHz  = getValue (parameters, params::lowCut);
    const float highCutHz = getValue (parameters, params::highCut);
    const float transAmt  = juce::jlimit (0.0f, 1.5f, getValue (parameters, params::transient) + profile.transientBias);
    const bool  sideMute  = getValue (parameters, params::sideMute) > 0.5f;

    colorSm.setTargetValue   (getValue (parameters, params::color));
    amountSm.setTargetValue  (getValue (parameters, params::amount));
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
    // Scale (0) and UI (3) use the scale mask.

    fillTargetNotes (targets, mask, shift, maxVoices);
    colourCore.setTargets (targets, maxVoices);

    uiHeldMask.store (midiMask);
    uiTargetMask.store (mask);
    uiVoiceCount.store (colourCore.getActiveVoiceCount());

    // ── Affected range split ──────────────────────────────────────────────────
    affectedRange.setRange (lowCutHz, highCutHz);
    uiLowHz.store (affectedRange.getLowHz());
    uiHighHz.store (affectedRange.getHighHz());

    float* dry[2]; float* active[2]; float* tuned[2];
    const float* in[2];
    for (int ch = 0; ch < numCh; ++ch)
    {
        dry[ch]    = dryBuf.getWritePointer (ch);
        active[ch] = activeBuf.getWritePointer (ch);
        tuned[ch]  = tunedBuf.getWritePointer (ch);
        in[ch]     = buffer.getReadPointer (ch);
        std::copy (in[ch], in[ch] + numSamples, dry[ch]);
    }

    affectedRange.process (dry, active, numCh, numSamples);

    for (int ch = 0; ch < numCh; ++ch)
        std::copy (active[ch], active[ch] + numSamples, tuned[ch]); // core works on the copy

    // Transient envelope on the dry active band.
    float flash = 0.0f;
    transientDetector.process (active, numCh, numSamples, transientEnv.data(), flash);
    uiTransientFlash.store (flash);

    // ── Colour mapping core ───────────────────────────────────────────────────
    // Advance the block-rate smoothers (skip = step by numSamples and return the
    // new value) so COLOR / Amount / Gate actually track the knobs.
    const float colorNow = colorSm.skip (numSamples);
    ColourMappingCore::Settings cset;
    cset.color01    = juce::jmin (colorNow, 1.0f);
    cset.colorBoost = juce::jlimit (0.0f, 1.0f, colorNow - 1.0f);
    cset.amount     = amountSm.skip (numSamples);
    cset.gate       = gateSm.skip (numSamples);
    cset.profile    = profile;

    ColourMappingCore::Energies energies;
    colourCore.process (tuned, active, numCh, numSamples, cset, energies);

    uiInputEnergy.store (energies.input);
    uiTunedEnergy.store (energies.tuned);
    uiColoredEnergy.store (energies.colored);

    // ── Pseudo formant / gamma ────────────────────────────────────────────────
    const float formantSt  = formantSm.skip (numSamples);
    const float gammaNow   = gammaSm.skip (numSamples);
    const float formantAmt = juce::jlimit (0.0f, 1.0f,
                                          (std::abs (formantSt) / 24.0f * 0.6f + gammaNow * 0.6f)
                                              * (0.5f + 0.5f * profile.formantReact));
    formantTone.update (formantSt, formantAmt);
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
        const float tg    = juce::jlimit (0.0f, 1.0f, transientEnv[(size_t) s] * transAmt);

        for (int ch = 0; ch < numCh; ++ch)
        {
            const float act  = active[ch][s];
            const float proc = tuned[ch][s] + tg * (act - tuned[ch][s]); // attacks pass dry
            float outSample  = dry[ch][s] + mix * (proc - act);          // out-of-range stays dry
            outSample *= outGn;
            buffer.setSample (ch, s, outSample);
        }
    }

    float* out[2];
    for (int ch = 0; ch < numCh; ++ch)
        out[ch] = buffer.getWritePointer (ch);
    limiter.process (out, numCh, numSamples);

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
            parameters.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new NSColourMapAudioProcessor();
}
