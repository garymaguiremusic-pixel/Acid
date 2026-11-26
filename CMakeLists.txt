cmake_minimum_required(VERSION 3.22)

# ------------------------------------------------------------------------------
# The plugin's name lives here only: change it and everything follows.
set(ACID_PLUGIN_NAME "Acid" CACHE STRING "Plugin product name")
set(ACID_TARGET Acid)

# macOS: universal binary (Apple Silicon + Intel) that runs on 10.13 and later.
# (Set before project(), where APPLE is not known yet.)
if(CMAKE_HOST_APPLE)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "10.13" CACHE STRING "Minimum macOS version")
    if(NOT CMAKE_OSX_ARCHITECTURES)
        set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "macOS architectures" FORCE)
    endif()
endif()

project(${ACID_TARGET} VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

option(ACID_BUILD_PLUGIN     "Build the plugin (needs JUCE)"                 ON)
option(ACID_BUILD_STANDALONE "Also build a standalone app for testing"       OFF)
option(ACID_BUILD_AU         "Also build the Audio Unit (macOS only)"        ON)
option(ACID_BUILD_TESTS      "Build the DSP / sequencer unit tests (Catch2)" ON)
option(ACID_COPY_AFTER_BUILD "Install the plugin into the user plugin folder after building" OFF)
set(JUCE_DIR "" CACHE PATH "Path to a local JUCE checkout (leave empty to download JUCE 8.0.4)")

include(FetchContent)

# ------------------------------------------------------------------------------
# Engine: plain C++ (no JUCE), shared by the plugin and the tests.
add_library(AcidEngine STATIC
    src/dsp/Oscillator.cpp
    src/dsp/Ladder.cpp
    src/dsp/Envelopes.cpp
    src/dsp/Voice.cpp
    src/dsp/Lfo.cpp
    src/dsp/ToneStack.cpp
    src/dsp/MultibandTube.cpp
    src/dsp/Echo.cpp
    src/dsp/Voice303.cpp
    src/dsp/Distortion.cpp)
target_include_directories(AcidEngine PUBLIC src)
set_target_properties(AcidEngine PROPERTIES POSITION_INDEPENDENT_CODE ON)
if(MSVC)
    target_compile_options(AcidEngine PRIVATE /W4 /fp:fast)
else()
    target_compile_options(AcidEngine PRIVATE -Wall -Wextra -O2)
endif()

# ------------------------------------------------------------------------------
# Plugin
if(ACID_BUILD_PLUGIN)
    if(JUCE_DIR)
        add_subdirectory(${JUCE_DIR} ${CMAKE_BINARY_DIR}/JUCE EXCLUDE_FROM_ALL)
    else()
        FetchContent_Declare(JUCE
            GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
            GIT_TAG        8.0.4
            GIT_SHALLOW    ON)
        FetchContent_MakeAvailable(JUCE)
    endif()

    set(ACID_FORMATS VST3)
    if(APPLE AND ACID_BUILD_AU)
        list(APPEND ACID_FORMATS AU)
    endif()
    if(ACID_BUILD_STANDALONE)
        list(APPEND ACID_FORMATS Standalone)
    endif()

    juce_add_plugin(${ACID_TARGET}
        PRODUCT_NAME                "${ACID_PLUGIN_NAME}"
        COMPANY_NAME                "Acid Project"
        BUNDLE_ID                   com.acidproject.acid
        PLUGIN_MANUFACTURER_CODE    Acpj
        PLUGIN_CODE                 Acd1
        FORMATS                     ${ACID_FORMATS}
        IS_SYNTH                    TRUE
        NEEDS_MIDI_INPUT            TRUE
        NEEDS_MIDI_OUTPUT           TRUE
        IS_MIDI_EFFECT              FALSE
        EDITOR_WANTS_KEYBOARD_FOCUS FALSE
        VST3_CATEGORIES             Instrument Synth
        AU_MAIN_TYPE                kAudioUnitType_MusicDevice
        HARDENED_RUNTIME_ENABLED    TRUE
        COPY_PLUGIN_AFTER_BUILD     ${ACID_COPY_AFTER_BUILD})

    target_sources(${ACID_TARGET} PRIVATE
        src/PluginProcessor.cpp
        src/PluginEditor.cpp
        src/Parameters.cpp
        src/arp/ArpEngine.cpp
        src/arp/NoteOrder.cpp
        src/arp/Scales.cpp
        src/arp/Randomiser.cpp
        src/arp/PatternModel.cpp
        src/arp/import/PatternImport.cpp
        src/arp/import/AudioToNotes.cpp
        src/arp/import/PitchDetector.cpp
        src/arp/export/MidiRenderer.cpp
        src/presets/PresetManager.cpp
        src/presets/FactoryPresets.cpp
        src/ui/LookAndFeel.cpp
        src/ui/Knob.cpp
        src/ui/Widgets.cpp
        src/ui/arp/ArpStepGrid.cpp
        src/ui/arp/DragMidiHandle.cpp
        src/ui/arp/Controls.cpp
        src/ui/arp/VirusWidgets.cpp
        src/ui/pages/SequencerPage.cpp
        src/ui/pages/SeqPanel.cpp
        src/ui/pages/Page303.cpp
        src/ui/BandGraph.cpp
        src/ui/TubeView.cpp
        src/ui/pages/SynthPage.cpp
        src/ui/pages/LayerRow.cpp
        src/ui/pages/AmpEnvPanel.cpp
        src/ui/pages/TubePage.cpp)

    target_compile_definitions(${ACID_TARGET} PUBLIC
        ACID_PRODUCT_NAME="${ACID_PLUGIN_NAME}"
        JUCE_USE_MP3AUDIOFORMAT=1
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_DISPLAY_SPLASH_SCREEN=0)

    target_link_libraries(${ACID_TARGET}
        PRIVATE
            AcidEngine
            juce::juce_audio_utils
            juce::juce_audio_formats
        PUBLIC
            juce::juce_recommended_config_flags
            juce::juce_recommended_lto_flags
            juce::juce_recommended_warning_flags)
endif()

# ------------------------------------------------------------------------------
# Tests
if(ACID_BUILD_TESTS)
    FetchContent_Declare(Catch2
        GIT_REPOSITORY https://github.com/catchorg/Catch2.git
        GIT_TAG        v3.7.1
        GIT_SHALLOW    ON)
    FetchContent_MakeAvailable(Catch2)

    add_executable(AcidTests
        tests/TestDsp.cpp
        tests/TestTube.cpp)
    target_link_libraries(AcidTests PRIVATE AcidEngine Catch2::Catch2WithMain)

    enable_testing()
    add_test(NAME AcidTests COMMAND AcidTests)

    # The sequencer (from ArpSampler) uses JUCE's core / MIDI classes, so it has its own test app.
    if(ACID_BUILD_PLUGIN)
        juce_add_console_app(AcidSeqTests PRODUCT_NAME "AcidSeqTests")
        target_sources(AcidSeqTests PRIVATE
            tests/TestArpSequencer.cpp
            src/arp/ArpEngine.cpp
            src/arp/NoteOrder.cpp
            src/arp/Scales.cpp
            src/arp/Randomiser.cpp
            src/arp/import/PatternImport.cpp
            src/arp/export/MidiRenderer.cpp
            src/presets/FactoryPresets.cpp)
        target_include_directories(AcidSeqTests PRIVATE src)
        target_compile_definitions(AcidSeqTests PRIVATE ACID_PRODUCT_NAME="${ACID_PLUGIN_NAME}" JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0)
        target_link_libraries(AcidSeqTests PRIVATE juce::juce_audio_basics juce::juce_core Catch2::Catch2WithMain)
        add_test(NAME AcidSeqTests COMMAND AcidSeqTests)
    endif()
endif()

# ------------------------------------------------------------------------------
# Offline render / screenshot check (developer tool, off by default)
option(ACID_BUILD_TOOLS "Build the RenderCheck developer tool" OFF)
if(ACID_BUILD_PLUGIN AND ACID_BUILD_TOOLS)
    juce_add_console_app(RenderCheck PRODUCT_NAME "RenderCheck")
    target_sources(RenderCheck PRIVATE tools/RenderCheck.cpp)
    target_compile_definitions(RenderCheck PRIVATE JUCE_MODAL_LOOPS_PERMITTED=1 JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 ACID_PRODUCT_NAME="${ACID_PLUGIN_NAME}"
        JucePlugin_Name="${ACID_PLUGIN_NAME}")
    target_link_libraries(RenderCheck PRIVATE ${ACID_TARGET} juce::juce_audio_utils juce::juce_audio_formats)
endif()
