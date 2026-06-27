#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <atomic>
#include <vector>

#include "Parameters.h"
#include "dsp/CharacterModes.h"
#include "dsp/MidiChordState.h"
#include "dsp/ScaleNoteSet.h"
#include "dsp/TargetNoteGenerator.h"
#include "dsp/AffectedRange.h"
#include "dsp/TransientDetector.h"
#include "dsp/ColourMappingCore.h"
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
    double getTailLengthSeconds() const override { return 1.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    APVTS& getState() { return parameters; }

    // ── UI state (lock-free) ──────────────────────────────────────────────────
    float getMidiActivity()   const noexcept { return uiMidiActivity.load(); }
    float getTransientFlash() const noexcept { return uiTransientFlash.load(); }
    float getInputEnergy()    const noexcept { return uiInputEnergy.load(); }
    float getTunedEnergy()    const noexcept { return uiTunedEnergy.load(); }
    float getColoredEnergy()  const noexcept { return uiColoredEnergy.load(); }
    int   getActiveVoiceCount() const noexcept { return uiVoiceCount.load(); }
    float getLowHz()  const noexcept { return uiLowHz.load(); }
    float getHighHz() const noexcept { return uiHighHz.load(); }
    juce::uint32 getTargetMask() const noexcept { return uiTargetMask.load(); }
    juce::uint32 getHeldMask()   const noexcept { return uiHeldMask.load(); }

private:
    int qualityToMaxVoices (int quality) const noexcept;

    APVTS parameters;
    double currentSampleRate = 44100.0;

    nscm::MidiChordState    chordState;
    nscm::AffectedRange     affectedRange;
    nscm::TransientDetector transientDetector;
    nscm::ColourMappingCore colourCore;
    nscm::PseudoFormantTone formantTone;
    nscm::SafetyLimiter     limiter;
    nscm::TargetNoteList    targets;

    juce::AudioBuffer<float> dryBuf, activeBuf, tunedBuf;
    std::vector<float>       transientEnv;

    juce::SmoothedValue<float> colorSm, amountSm, mixSm, outputSm, formantSm, gammaSm, gateSm;
    juce::SmoothedValue<float> colourGain; // master colour fade (0 when no grid -> stops on note-off)

    std::atomic<float>        uiMidiActivity { 0.0f };
    std::atomic<float>        uiTransientFlash { 0.0f };
    std::atomic<float>        uiInputEnergy { 0.0f };
    std::atomic<float>        uiTunedEnergy { 0.0f };
    std::atomic<float>        uiColoredEnergy { 0.0f };
    std::atomic<int>          uiVoiceCount { 0 };
    std::atomic<float>        uiLowHz { 100.0f };
    std::atomic<float>        uiHighHz { 6000.0f };
    std::atomic<juce::uint32> uiTargetMask { 0 };
    std::atomic<juce::uint32> uiHeldMask { 0 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (NSColourMapAudioProcessor)
};
