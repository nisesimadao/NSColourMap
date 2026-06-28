cmake_minimum_required(VERSION 3.22)

project(NSColourMap VERSION 0.8.6 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_OSX_DEPLOYMENT_TARGET "10.15" CACHE STRING "Minimum macOS version")

option(NSCM_FETCH_JUCE "Download JUCE with FetchContent when JUCE is not already available" ON)

find_package(JUCE CONFIG QUIET)

if(NOT JUCE_FOUND)
    if(NOT NSCM_FETCH_JUCE)
        message(FATAL_ERROR "JUCE was not found. Set JUCE_DIR or enable NSCM_FETCH_JUCE.")
    endif()

    include(FetchContent)
    FetchContent_Declare(
        JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG 8.0.14
        GIT_SHALLOW TRUE
    )
    FetchContent_MakeAvailable(JUCE)
endif()

juce_add_plugin(NSColourMap
    VERSION ${PROJECT_VERSION}
    COMPANY_NAME "nisesimadao"
    BUNDLE_ID "com.nscolourmap.plugin"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD TRUE
    PLUGIN_MANUFACTURER_CODE Nscm
    PLUGIN_CODE Nscm
    FORMATS VST3 AU
    PRODUCT_NAME "NSColourMap"
)

target_sources(NSColourMap
    PRIVATE
        Source/Parameters.cpp
        Source/PluginEditor.cpp
        Source/PluginProcessor.cpp
)

target_include_directories(NSColourMap
    PRIVATE
        Source
)

target_compile_definitions(NSColourMap
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
)

target_link_libraries(NSColourMap
    PRIVATE
        juce::juce_audio_processors
        juce::juce_audio_utils
        juce::juce_dsp
        juce::juce_gui_basics
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

# ── DSP smoke test (header-only, no JUCE dependency) ─────────────────────────
add_executable(NSColourMap_SmokeTest tests/DspSmoke.cpp)
target_include_directories(NSColourMap_SmokeTest PRIVATE Source)
set_target_properties(NSColourMap_SmokeTest PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)
enable_testing()
add_test(NAME SmokeTest COMMAND NSColourMap_SmokeTest)

# ── Offline full-chain check (compiles the real processor with JUCE) ─────────
add_executable(NSColourMap_OfflineCheck
    tests/OfflineCheck.cpp
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
    Source/Parameters.cpp)
target_include_directories(NSColourMap_OfflineCheck PRIVATE Source)
target_compile_definitions(NSColourMap_OfflineCheck PRIVATE
    JUCE_STANDALONE_APPLICATION=1
    JucePlugin_Name="NSColourMap"
    JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0)
target_link_libraries(NSColourMap_OfflineCheck PRIVATE
    juce::juce_audio_utils
    juce::juce_audio_processors
    juce::juce_dsp
    juce::juce_gui_basics
    juce::juce_recommended_config_flags)

# ── Screenshot generator (renders the editor to PNGs for the README) ─────────
add_executable(NSColourMap_Screenshot
    tools/Screenshot.cpp
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
    Source/Parameters.cpp)
target_include_directories(NSColourMap_Screenshot PRIVATE Source)
target_compile_definitions(NSColourMap_Screenshot PRIVATE
    JUCE_STANDALONE_APPLICATION=1
    JUCE_MODAL_LOOPS_PERMITTED=1
    JucePlugin_Name="NSColourMap"
    JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0)
target_link_libraries(NSColourMap_Screenshot PRIVATE
    juce::juce_audio_utils
    juce::juce_audio_processors
    juce::juce_dsp
    juce::juce_gui_basics
    juce::juce_graphics
    juce::juce_recommended_config_flags)
