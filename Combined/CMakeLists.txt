# ==============================================================================
#  MacDanial Hills Mastering Compressor - Model A, Model B and bus compressor C stacked (Basemental)
#  Optical -> Class-A Discrete VCA, Lundahl-style input transformer,
#  switchable Nickel / Iron / Steel output transformers.  JUCE 8, CMake.
# ==============================================================================
cmake_minimum_required(VERSION 3.22)

# macOS: build a Universal Binary (Apple Silicon + Intel). Must precede project().
if(APPLE)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "10.13" CACHE STRING "Minimum macOS version")
    set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "Build architectures")
endif()

project(MacDanialHillsStack VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ---- Fetch JUCE ---------------------------------------------------------------
include(FetchContent)
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG        8.0.4
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(JUCE)

# ---- Formats: AU only exists on macOS ----------------------------------------
if(APPLE)
    set(MDH_FORMATS AU VST3 Standalone)
else()
    set(MDH_FORMATS VST3 Standalone)
endif()

juce_add_plugin(MacDanialHillsStack
    COMPANY_NAME                "Basemental"
    BUNDLE_ID                   "com.basemental.macdanialhills"
    PRODUCT_NAME                "MacDanial Hills Mastering Compressor"
    PLUGIN_MANUFACTURER_CODE    Bsmt          # 4 chars, at least one uppercase
    PLUGIN_CODE                 Mdhc          # 4 chars, unique per plugin
    FORMATS                     ${MDH_FORMATS}
    IS_SYNTH                    FALSE
    NEEDS_MIDI_INPUT            FALSE
    NEEDS_MIDI_OUTPUT           FALSE
    IS_MIDI_EFFECT              FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    VST3_CATEGORIES             Fx Dynamics
    AU_MAIN_TYPE                kAudioUnitType_Effect
    COPY_PLUGIN_AFTER_BUILD     TRUE          # installs into your system plug-in folders
)

target_sources(MacDanialHillsStack PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
    Source/DSP/EngineA.cpp
    Source/DSP/EngineB.cpp
    Source/UnitPanelA.cpp
    Source/UnitPanelB.cpp
    Source/UnitPanelC.cpp
    Source/Presets.cpp
    Source/PopOutWindow.cpp
    Source/DSP/BusComp.cpp
    Source/UI/PanelLookAndFeel.cpp
    Source/UI/PanelComponents.cpp
)

target_include_directories(MacDanialHillsStack PRIVATE Source)

target_compile_definitions(MacDanialHillsStack PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0
)

target_link_libraries(MacDanialHillsStack
    PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)
