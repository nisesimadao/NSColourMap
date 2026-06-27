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
    switch (quality)
    {
        case 0:  return 12; // Eco
        case 2:  return 32; // High
        default: return 24; // Normal
    }
}

void NSColourMapAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate > 0.0 ? sampleRate : 44100.0;
    const int numCh = juce::jmax (getTotalNumInputChannels(), getTotalNumOutputChannels(), 2);
    const int block = juce::jmax (16, samplesPerBlock);

    subSplitter.prepare (currentSampleRate, block, numCh);
    transientDetector.prepare (currentSampleRate);
    resonatorBank.prepare (currentSampleRate, block);
    colourProcessor.prepare (currentSampleRate);
    formantTone.prepare (currentSampleRate);
    limiter.prepare (currentSampleRate);
    chordState.reset();

    dryBuf.setSize (numCh, block, false, false, true);
    lowBuf.setSize (numCh, block, false, false, true);
    highBuf.setSize (numCh, block, false, false, true);
    dryHighBuf.setSize (numCh, block, false, false, true);
    transientEnv.assign ((size_t) block, 0.0f);

    const double ramp = 0.02;
    mixSmoothed.reset (currentSampleRate, ramp);
    outputSmoothed.reset (currentSampleRate, ramp);
    amountSmoothed.reset (currentSampleRate, ramp);
    colourSmoothed.reset (currentSampleRate, ramp);
    formantSmoothed.reset (currentSampleRate, 0.05);

    mixSmoothed.setCurrentAndTargetValue (getValue (parameters, params::mix));
    outputSmoothed.setCurrentAndTargetValue (juce::Decibels::decibelsToGain (getValue (parameters, params::output)));
    amountSmoothed.setCurrentAndTargetValue (getValue (parameters, params::amount));
    colourSmoothed.setCurrentAndTargetValue (getValue (parameters, params::colour));
    formantSmoothed.setCurrentAndTargetValue (getValue (parameters, params::formant));
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
    const int   modeIdx    = (int) getChoice (parameters, params::mode, 0);
    const auto  algo       = (Algo) (int) getChoice (parameters, params::algo, 1);
    const auto& profile    = getAlgoProfile (algo);
    const bool  freeze     = getValue (parameters, params::freezeChord) > 0.5f;
    const int   keyIdx     = (int) getChoice (parameters, params::key, 0);
    const auto  scaleType  = (Scale) (int) getChoice (parameters, params::scale, 7);
    const int   shift      = (int) std::lround (getValue (parameters, params::scaleShift));
    const int   maxVoices  = qualityToMaxVoices ((int) getChoice (parameters, params::quality, 1));
    const float subHz      = getValue (parameters, params::subProtect);
    const float glideMs    = getValue (parameters, params::glide) * profile.glideScale;
    const float transAmt   = juce::jlimit (0.0f, 1.5f, getValue (parameters, params::transient) + profile.transientBias);

    amountSmoothed.setTargetValue (getValue (parameters, params::amount));
    colourSmoothed.setTargetValue (getValue (parameters, params::colour));
    formantSmoothed.setTargetValue (getValue (parameters, params::formant));
    mixSmoothed.setTargetValue (getValue (parameters, params::mix));
    outputSmoothed.setTargetValue (juce::Decibels::decibelsToGain (getValue (parameters, params::output)));

    // ── MIDI handling ─────────────────────────────────────────────────────────
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

    // ── Target generation ─────────────────────────────────────────────────────
    juce::uint16 targetMask = 0;
    if (modeIdx == 0) // MIDI Chord
        targetMask = chordState.getActiveMask (freeze);
    else              // Scale Resonance
        targetMask = makeScaleNoteSet (keyIdx, scaleType).mask;

    fillTargetNotes (targets, targetMask, shift, maxVoices);
    resonatorBank.setTargets (targets, maxVoices);

    uiHeldMask.store (chordState.getActiveMask (false));
    uiTargetMask.store (targetMask);
    uiVoiceCount.store (resonatorBank.getActiveCount());
    uiSubHz.store (subHz);

    // ── Sub split ─────────────────────────────────────────────────────────────
    subSplitter.setCrossoverFrequency (subHz);

    float* dry[2];  float* low[2];  float* high[2];  float* dryHigh[2];
    const float* in[2];
    for (int ch = 0; ch < numCh; ++ch)
    {
        dry[ch]     = dryBuf.getWritePointer (ch);
        low[ch]     = lowBuf.getWritePointer (ch);
        high[ch]    = highBuf.getWritePointer (ch);
        dryHigh[ch] = dryHighBuf.getWritePointer (ch);
        in[ch]      = buffer.getReadPointer (ch);
        std::copy (in[ch], in[ch] + numSamples, dry[ch]); // original signal
    }

    subSplitter.process (dry, low, high, numCh, numSamples);

    // Keep a dry copy of the high band for transient return + dry/wet.
    for (int ch = 0; ch < numCh; ++ch)
        std::copy (high[ch], high[ch] + numSamples, dryHigh[ch]);

    // Transient envelope (on the dry high band).
    float flash = 0.0f;
    transientDetector.process (dryHigh, numCh, numSamples, transientEnv.data(), flash);
    uiTransientFlash.store (flash);

    // ── Wet chain: Resonator -> Colour -> Pseudo Formant ───────────────────────
    ResonatorBank::Tuning rt;
    const float colourNow = colourSmoothed.getCurrentValue();
    rt.q           = profile.qBase + colourNow * profile.qColour;
    rt.drive       = profile.driveBase;
    rt.stereoCents = profile.stereoCents;
    rt.baseGain    = profile.wetScale;
    const float glideSec = glideMs * 0.001f;
    rt.glideCoeff  = glideSec <= 0.0f ? 0.0f
                                      : std::exp (-((float) numSamples / (float) currentSampleRate) / glideSec);
    resonatorBank.process (high, numCh, numSamples, rt);

    ColourProcessor::Settings cs;
    cs.colour   = colourNow;
    cs.drive    = profile.driveBase;
    cs.width    = profile.widthScale;
    cs.highEmph = profile.highEmph;
    colourProcessor.process (high, numCh, numSamples, cs);

    const float formantSt    = formantSmoothed.getCurrentValue();
    const float formantAmount = juce::jlimit (0.0f, 1.0f,
                                              std::abs (formantSt) / 24.0f * 0.5f
                                                  + colourNow * profile.formantScale * 0.4f);
    formantTone.update (formantSt, formantAmount);
    formantTone.process (high, numCh, numSamples);

    // ── Combine: amount, transient return, mix, output ─────────────────────────
    float energy = 0.0f;
    for (int s = 0; s < numSamples; ++s)
    {
        const float amount = amountSmoothed.getNextValue();
        const float mix    = mixSmoothed.getNextValue();
        const float outGn  = outputSmoothed.getNextValue();
        const float tg     = juce::jlimit (0.0f, 1.0f, transientEnv[(size_t) s] * transAmt);

        for (int ch = 0; ch < numCh; ++ch)
        {
            const float dh = dryHigh[ch][s];

            // Effect depth (dry high -> processed high).
            float effHigh = dh + amount * (high[ch][s] - dh);

            // Transient return: let attacks pass through dry.
            effHigh = effHigh + tg * (dh - effHigh);

            const float wet = low[ch][s] + effHigh;   // protected sub stays dry
            const float dryFull = dry[ch][s];
            float outSample = dryFull + mix * (wet - dryFull);
            outSample *= outGn;

            buffer.setSample (ch, s, outSample);
            energy += dryFull * dryFull;
        }
    }

    float* out[2];
    for (int ch = 0; ch < numCh; ++ch)
        out[ch] = buffer.getWritePointer (ch);
    limiter.process (out, numCh, numSamples);

    uiInputEnergy.store (std::sqrt (energy / (float) (numSamples * numCh)));
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
