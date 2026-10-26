cmake_minimum_required(VERSION 3.22)
project(VelvetPad VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(JUCE)

juce_add_plugin(VelvetPad
    COMPANY_NAME "MS Audio"
    PRODUCT_NAME "Velvet Pad"
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD TRUE
    PLUGIN_MANUFACTURER_CODE Msau
    PLUGIN_CODE Slks
    FORMATS VST3 AU Standalone
    VST3_CATEGORIES Instrument Synth
    AU_MAIN_TYPE kAudioUnitType_MusicDevice)

target_sources(VelvetPad PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
    Source/Graphs.cpp
    Source/FxChain.cpp
    Source/Visualizer.cpp)

target_compile_definitions(VelvetPad PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0)

target_link_libraries(VelvetPad
    PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)
