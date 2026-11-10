# ==============================================================================
#  Basemental OD-2 Mastering Compressor
#  Dual-stage (Optical -> Discrete VCA) mastering compressor with switchable
#  Nickel / Iron / Steel output transformers.  JUCE 8, CMake.
# ==============================================================================
cmake_minimum_required(VERSION 3.22)

# macOS: build a Universal Binary (Apple Silicon + Intel). Must precede project().
if(APPLE)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "10.13" CACHE STRING "Minimum macOS version")
    set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "Build architectures")
endif()

project(BasementalOD2 VERSION 1.0.0)

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
    set(OD2_FORMATS AU VST3 Standalone)
else()
    set(OD2_FORMATS VST3 Standalone)
endif()

juce_add_plugin(BasementalOD2
    COMPANY_NAME                "Basemental"
    BUNDLE_ID                   "com.basemental.macdanialhills.a"
    PRODUCT_NAME                "MacDanial Hills A"
    PLUGIN_MANUFACTURER_CODE    Bsmt          # 4 chars, at least one uppercase
    PLUGIN_CODE                 Mdha          # 4 chars, unique per plugin
    FORMATS                     ${OD2_FORMATS}
    IS_SYNTH                    FALSE
    NEEDS_MIDI_INPUT            FALSE
    NEEDS_MIDI_OUTPUT           FALSE
    IS_MIDI_EFFECT              FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    VST3_CATEGORIES             Fx Dynamics
    AU_MAIN_TYPE                kAudioUnitType_Effect
    COPY_PLUGIN_AFTER_BUILD     TRUE          # installs into your system plug-in folders
)

target_sources(BasementalOD2 PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
    Source/DSP/OD2Engine.cpp
    Source/UI/PanelLookAndFeel.cpp
    Source/UI/PanelComponents.cpp
)

target_include_directories(BasementalOD2 PRIVATE Source)

target_compile_definitions(BasementalOD2 PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0
)

target_link_libraries(BasementalOD2
    PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)
