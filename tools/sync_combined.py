#!/usr/bin/env python3
"""Regenerate Combined/ sources from the standalone A/ and B/ plug-ins.

Edit A/ and B/ (engines, editors) and the shared UI in Combined/Source/UI, then run:
    python3 tools/sync_combined.py
It copies the shared UI into A/ and B/, puts each engine in its own namespace
(od2a / od2b) and turns each editor into a stackable panel (UnitPanelA / UnitPanelB).
"""
import os, re, shutil

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
J = lambda *p: os.path.join(ROOT, *p)

def sub(s, old, new, all_=False):
    assert old in s, "pattern not found: " + old[:80]
    return s.replace(old, new) if all_ else s.replace(old, new, 1)

# 1. shared UI -> A and B
for u in ("A", "B"):
    for f in os.listdir(J("Combined", "Source", "UI")):
        shutil.copy(J("Combined", "Source", "UI", f), J(u, "Source", "UI", f))
shutil.copy(J("A", "Source", "ParamIDs.h"), J("Combined", "Source", "ParamIDs.h"))

# 2. engines -> namespaced copies
for u in ("A", "B"):
    ns = "od2" + u.lower()
    for ext in ("h", "cpp"):
        s = open(J(u, "Source", "DSP", "OD2Engine." + ext)).read()
        s = re.sub(r"^namespace od2$", "namespace " + ns, s, flags=re.M)
        s = re.sub(r"^\} // namespace od2$", "} // namespace " + ns, s, flags=re.M)
        s = s.replace('#include "OD2Engine.h"', '#include "Engine%s.h"' % u)
        open(J("Combined", "Source", "DSP", "Engine%s.%s" % (u, ext)), "w").write(s)

# 3. editors -> panels
for u in ("A", "B"):
    name, ns = "UnitPanel" + u, "od2" + u.lower()
    h = open(J(u, "Source", "PluginEditor.h")).read()
    c = open(J(u, "Source", "PluginEditor.cpp")).read()

    h = sub(h, '#include "PluginProcessor.h"', '#include "DSP/Engine%s.h"\n#include "ParamIDs.h"' % u)
    h = h.replace("OD2Editor", name)
    h = sub(h, "public juce::AudioProcessorEditor, private juce::Timer", "public juce::Component, private juce::Timer")
    h = sub(h, "    explicit %s (OD2AudioProcessor&);" % name,
            "    %s (juce::AudioProcessorValueTreeState&, %s::Engine&, juce::String paramPrefix);" % (name, ns))
    h = sub(h, "    OD2AudioProcessor& proc;",
            "    juce::AudioProcessorValueTreeState& apvts;\n    %s::Engine& engine;" % ns)

    c = sub(c, '#include "PluginEditor.h"', '#include "%s.h"' % name)
    c = c.replace("OD2Editor", name)
    c = sub(c, "using namespace od2;", "using namespace od2;\nusing namespace %s;" % ns)
    c = sub(c, "%s::%s (OD2AudioProcessor& p)\n    : AudioProcessorEditor (&p), proc (p)" % (name, name),
            "%s::%s (AudioProcessorValueTreeState& state, %s::Engine& eng, String paramPrefix)\n"
            "    : apvts (state), engine (eng), prefix (std::move (paramPrefix))" % (name, name, ns))
    c = c.replace("proc.apvts", "apvts").replace("proc.engine", "engine")
    c = sub(c, "auto id = [ch] (const char* base) { return ids::ch (ch, base); };",
            "auto id = [this, ch] (const char* base) { return prefix + ids::ch (ch, base); };")
    c = re.sub(r"(?<!prefix \+ )ids::hardwireBypass", "prefix + ids::hardwireBypass", c)
    c = c.replace("(ids::stereo)", "(prefix + ids::stereo)").replace(", ids::stereo,", ", prefix + ids::stereo,")
    c = re.sub(r"    // ---- window -+\n.*?setSize \(1200, 540\);\n", "", c, flags=re.S)
    assert "setResizable" not in c and "proc." not in c
    open(J("Combined", "Source", name + ".h"), "w").write(h)
    open(J("Combined", "Source", name + ".cpp"), "w").write(c)

print("Combined/ regenerated from A/ and B/")
