#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <array>
#include <atomic>
#include <vector>

#include "Parameters.h"
#include "dsp/AlgoModes.h"
#include "dsp/MidiChordState.h"
#include "dsp/ScaleNoteSet.h"
#include "dsp/TargetNoteGenerator.h"
#include "dsp/SubSplitter.h"
#include "dsp/TransientDetector.h"
#include "dsp/ResonatorBank.h"
#include "dsp/ColourProcessor.h"
#include "dsp/PseudoFormantTone.h"
#include "dsp/SafetyLimiter.h"

class NSColourMapAudioProcessor final : public juce::AudioProcessor
{
public:
    using APVTS = juce::AudioProcessorValueTreeState;

    NSColourMapAudioProcessor();
    ~NSColourMapAudioProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.5; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    APVTS& getState() { return parameters; }

    // ── Snapshots A/B/C/D (message-thread only) ───────────────────────────────
    static constexpr int kNumSnapshots = 4;
    void storeSnapshot (int index);
    bool recallSnapshot (int index);
    bool isSnapshotFilled (int index) const;

    // ── UI state (lock-free) ──────────────────────────────────────────────────
    float getMidiActivity()    const noexcept { return uiMidiActivity.load(); }
    float getTransientFlash()  const noexcept { return uiTransientFlash.load(); }
    float getInputEnergy()     const noexcept { return uiInputEnergy.load(); }
    int   getActiveVoiceCount() const noexcept { return uiVoiceCount.load(); }
    float getSubProtectHz()    const noexcept { return uiSubHz.load(); }
    juce::uint32 getTargetMask() const noexcept { return uiTargetMask.load(); }
    juce::uint32 getHeldMask()   const noexcept { return uiHeldMask.load(); }

private:
    int qualityToMaxVoices (int quality) const noexcept;

    APVTS parameters;
    double currentSampleRate = 44100.0;

    nscm::MidiChordState    chordState;
    nscm::SubSplitter       subSplitter;
    nscm::TransientDetector transientDetector;
    nscm::ResonatorBank     resonatorBank;
    nscm::ColourProcessor   colourProcessor;
    nscm::PseudoFormantTone formantTone;
    nscm::SafetyLimiter     limiter;
    nscm::TargetNoteList    targets;

    juce::AudioBuffer<float> dryBuf, lowBuf, highBuf, dryHighBuf;
    std::vector<float>       transientEnv;

    std::array<juce::ValueTree, kNumSnapshots> snapshots;

    // Smoothed macros
    juce::SmoothedValue<float> mixSmoothed, outputSmoothed, amountSmoothed, colourSmoothed, formantSmoothed;

    std::atomic<float>        uiMidiActivity { 0.0f };
    std::atomic<float>        uiTransientFlash { 0.0f };
    std::atomic<float>        uiInputEnergy { 0.0f };
    std::atomic<int>          uiVoiceCount { 0 };
    std::atomic<float>        uiSubHz { 120.0f };
    std::atomic<juce::uint32> uiTargetMask { 0 };
    std::atomic<juce::uint32> uiHeldMask { 0 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (NSColourMapAudioProcessor)
};
