cmake_minimum_required(VERSION 3.22)
project(VoidWave VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(JUCE)

# ── Embed all factory presets into the binary ─────────────────────────────────
# This makes the plugin self-contained — no source tree needed at runtime.
file(GLOB_RECURSE PRESET_SOURCES
    "${CMAKE_SOURCE_DIR}/Resources/Presets/factory/*.vwpreset"
)
juce_add_binary_data(VoidWave_Presets
    NAMESPACE  VWPresets
    HEADER_NAME "VoidWavePresets.h"
    SOURCES    ${PRESET_SOURCES}
             "${CMAKE_SOURCE_DIR}/Resources/flat-ui.png"
)

juce_add_plugin(VoidWave
    COMPANY_NAME               "Void Audio"
    BUNDLE_ID                  "com.VoidAudio.VoidWave"
    PLUGIN_MANUFACTURER_CODE   Vdwv
    PLUGIN_CODE                Vdw1
    FORMATS                    VST3 AU Standalone
    PRODUCT_NAME               "Void Wave"
    IS_SYNTH                   TRUE
    NEEDS_MIDI_INPUT           TRUE
    NEEDS_MIDI_OUTPUT          FALSE
    IS_MIDI_EFFECT             FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD    TRUE
    VST3_CATEGORIES            "Instrument|Synth"
    AU_MAIN_TYPE               kAudioUnitType_MusicDevice
)

target_sources(VoidWave PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp

    Source/DSP/WavetableOscillator.cpp
    Source/DSP/UnisonVoice.cpp
    Source/DSP/Filter.cpp
    Source/DSP/EnvelopeGenerator.cpp
    Source/DSP/LFO.cpp
    Source/DSP/ModMatrix.cpp
    Source/DSP/VoiceManager.cpp
    Source/DSP/FXChain/Distortion.cpp
    Source/DSP/FXChain/Chorus.cpp
    Source/DSP/FXChain/Delay.cpp
    Source/DSP/FXChain/Reverb.cpp
    Source/DSP/FXChain/Peverb/AllPassFilter.cpp
    Source/DSP/FXChain/Peverb/Delay.cpp
    Source/DSP/FXChain/Peverb/EarlyReflections.cpp
    Source/DSP/FXChain/Peverb/LateReverb.cpp
    Source/DSP/FXChain/Peverb/Nested.cpp
    Source/DSP/FXChain/Peverb/Processor.cpp
    Source/DSP/FXChain/Peverb/Wrap.cpp

    Source/UI/LookAndFeel.cpp
    Source/UI/OscSection.cpp
    Source/UI/FilterSection.cpp
    Source/UI/EnvelopeSection.cpp
    Source/UI/LFOSection.cpp
    Source/UI/ModMatrixSection.cpp
    Source/UI/MacroSection.cpp
    Source/UI/FXSection.cpp
    Source/UI/WavetableDisplay.cpp
    Source/UI/MidiLearnOverlay.cpp
    Source/UI/PresetBrowser.cpp

    Source/Data/WavetableBank.cpp
    Source/Data/PresetManager.cpp
)

juce_generate_juce_header(VoidWave)

target_include_directories(VoidWave PRIVATE Source)

target_compile_definitions(VoidWave PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0
    JUCE_REPORT_APP_USAGE=0
)

target_link_libraries(VoidWave PRIVATE
    VoidWave_Presets
    juce::juce_audio_basics
    juce::juce_audio_devices
    juce::juce_audio_formats
    juce::juce_audio_plugin_client
    juce::juce_audio_processors
    juce::juce_audio_utils
    juce::juce_core
    juce::juce_data_structures
    juce::juce_dsp
    juce::juce_events
    juce::juce_graphics
    juce::juce_gui_basics
    juce::juce_gui_extra
    juce::juce_opengl
    juce::juce_recommended_config_flags
    juce::juce_recommended_warning_flags
)

# ── Install rules (used by CPack and COPY_PLUGIN_AFTER_BUILD) ─────────────────
if(WIN32)
    install(
        DIRECTORY   "${CMAKE_BINARY_DIR}/VoidWave_artefacts/Release/VST3/Void Wave.vst3"
        DESTINATION "$ENV{COMMONPROGRAMFILES}/VST3"
    )
    install(
        PROGRAMS    "${CMAKE_BINARY_DIR}/VoidWave_artefacts/Release/Standalone/Void Wave.exe"
        DESTINATION "Void Audio/Void Wave"
    )
endif()

# ── CPack ─────────────────────────────────────────────────────────────────────
set(CPACK_PACKAGE_NAME              "Void Wave")
set(CPACK_PACKAGE_VENDOR            "Void Audio")
set(CPACK_PACKAGE_VERSION           "1.0.0")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Void Wave Wavetable Synthesizer VST3")
set(CPACK_PACKAGE_INSTALL_DIRECTORY "Void Audio/Void Wave")
set(CPACK_RESOURCE_FILE_LICENSE     "${CMAKE_SOURCE_DIR}/LICENSE.txt")

if(WIN32)
    set(CPACK_GENERATOR       "NSIS")
    set(CPACK_NSIS_DISPLAY_NAME   "Void Wave 1.0")
    set(CPACK_NSIS_PACKAGE_NAME   "Void Wave")
    set(CPACK_NSIS_INSTALL_ROOT   "$PROGRAMFILES64")
    set(CPACK_NSIS_MUI_ICON       "${CMAKE_SOURCE_DIR}/installer/icon.ico")
    set(CPACK_NSIS_ENABLE_UNINSTALL_BEFORE_INSTALL ON)
endif()

include(CPack)
