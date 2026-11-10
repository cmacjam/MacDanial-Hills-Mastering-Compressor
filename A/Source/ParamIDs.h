#pragma once
#include <juce_core/juce_core.h>

// Parameter IDs shared by the processor and the editor.
// Per-channel parameters are prefixed "L_" or "R_".
namespace od2::ids
{
    // Global
    inline constexpr const char* hardwireBypass = "BYPASS";   // true = OUT (hardwire bypassed)
    inline constexpr const char* stereo         = "STEREO";   // true = Stereo, false = Dual Mono

    // Per channel
    inline constexpr const char* optIn     = "OPT_IN";        // Optical section IN/OUT
    inline constexpr const char* optThr    = "OPT_THR";       // 1..24
    inline constexpr const char* optGain   = "OPT_GAIN";      // 0..24 (12 = unity)
    inline constexpr const char* dscIn     = "DSC_IN";        // Discrete section IN/OUT
    inline constexpr const char* dscThr    = "DSC_THR";       // 1..24
    inline constexpr const char* dscRatio  = "DSC_RATIO";     // 1.2 2 3 4 6 FLOOD
    inline constexpr const char* dscAttack = "DSC_ATTACK";    // .1 .5 1 5 10 30 ms
    inline constexpr const char* dscRecov  = "DSC_RECOVER";   // .1 .25 .5 .8 1.2 DUAL
    inline constexpr const char* dscGain   = "DSC_GAIN";      // 0..24 (12 = unity)
    inline constexpr const char* scFilter  = "SC_FILTER";     // 90 Hz sidechain HPF IN/OUT
    inline constexpr const char* xfmr      = "XFMR";          // Nickel / Iron / Steel
    inline constexpr const char* meterSel  = "METER";         // Optical / Discrete / Output

    inline juce::String ch (int channel, const char* base)
    {
        return juce::String (channel == 0 ? "L_" : "R_") + base;
    }
}
