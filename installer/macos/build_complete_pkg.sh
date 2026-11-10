#!/bin/bash
# One macOS installer (.pkg) with everything in it.
# Usage: build_complete_pkg.sh <stage-dir> <version> "<output.pkg>"
# <stage-dir> holds the unzipped bundles from the build jobs plus the manual (.html).
# Every part is selected by default; buyers can untick parts under "Customize".
set -euo pipefail

STAGE="$1"; VERSION="$2"; OUT="$3"
HERE="$(cd "$(dirname "$0")" && pwd)"
WORK="$(mktemp -d)"
ID="com.basemental.macdanialhills"
MAIN="MacDanial Hills Mastering Compressor"
OUTLINE=""; CHOICES=""; REFS=""

fix_relocatable () {   # never let macOS "relocate" a bundle into another copy found on disk
    local i=0
    while /usr/libexec/PlistBuddy -c "Print :$i" "$1" >/dev/null 2>&1; do
        /usr/libexec/PlistBuddy -c "Delete :$i:BundleIsRelocatable" "$1" >/dev/null 2>&1 || true
        /usr/libexec/PlistBuddy -c "Add :$i:BundleIsRelocatable bool false" "$1"
        i=$((i + 1))
    done
}

pkg_from_root () {     # root-dir install-location key
    local root="$1" dest="$2" key="$3"
    pkgbuild --analyze --root "$root" "$WORK/$key.plist" >/dev/null
    fix_relocatable "$WORK/$key.plist"
    pkgbuild --root "$root" --component-plist "$WORK/$key.plist" \
             --identifier "$ID.$key" --version "$VERSION" --install-location "$dest" "$WORK/$key.pkg"
    REFS+="    <pkg-ref id=\"$ID.$key\" version=\"$VERSION\">$key.pkg</pkg-ref>"$'\n'
}

pkg_bundle () {        # bundle install-location key   (skips silently if the bundle is missing)
    [ -e "$1" ] || return 1
    local root="$WORK/root-$3"
    mkdir -p "$root"
    cp -R "$1" "$root/"
    pkg_from_root "$root" "$2" "$3"
}

add_choice () {        # choice-id "title" "description" key...
    local cid="$1" title="$2" desc="$3"; shift 3
    OUTLINE+="        <line choice=\"$cid\"/>"$'\n'
    CHOICES+="    <choice id=\"$cid\" title=\"$title\" description=\"$desc\" start_selected=\"true\">"$'\n'
    for k in "$@"; do CHOICES+="        <pkg-ref id=\"$ID.$k\"/>"$'\n'; done
    CHOICES+="    </choice>"$'\n'
}

COMP="/Library/Audio/Plug-Ins/Components"
VST3="/Library/Audio/Plug-Ins/VST3"
AAXD="/Library/Application Support/Avid/Audio/Plug-Ins"

# ---- main plug-in (A + B + bus compressor) ----
pkg_bundle "$STAGE/$MAIN.component"  "$COMP" au   && add_choice au   "$MAIN - Audio Unit" "For Logic Pro and GarageBand. Installs to $COMP" au
pkg_bundle "$STAGE/$MAIN.vst3"       "$VST3" vst3 && add_choice vst3 "$MAIN - VST3" "For Ableton Live, Cubase, Studio One, Reaper, FL Studio, Bitwig. Installs to $VST3" vst3
if pkg_bundle "$STAGE/$MAIN.aaxplugin" "$AAXD" aax; then
    add_choice aax "$MAIN - AAX (Pro Tools)" "For Pro Tools. Installs to $AAXD" aax
fi

# ---- standalone app + manual, in /Applications/MacDanial Hills ----
if [ -d "$STAGE/$MAIN.app" ]; then
    APPROOT="$WORK/root-app/MacDanial Hills"
    mkdir -p "$APPROOT"
    cp -R "$STAGE/$MAIN.app" "$APPROOT/"
    [ -f "$STAGE/MacDanial Hills Manual.html" ] && cp "$STAGE/MacDanial Hills Manual.html" "$APPROOT/"
    pkg_from_root "$WORK/root-app" "/Applications" app
    add_choice app "Standalone app and user manual" "Runs without a DAW. Installs to /Applications/MacDanial Hills" app
fi

# ---- single-unit plug-ins ----
A_KEYS=(); B_KEYS=()
pkg_bundle "$STAGE/MacDanial Hills A.component" "$COMP" a-au   && A_KEYS+=(a-au)
pkg_bundle "$STAGE/MacDanial Hills A.vst3"      "$VST3" a-vst3 && A_KEYS+=(a-vst3)
pkg_bundle "$STAGE/MacDanial Hills B.component" "$COMP" b-au   && B_KEYS+=(b-au)
pkg_bundle "$STAGE/MacDanial Hills B.vst3"      "$VST3" b-vst3 && B_KEYS+=(b-vst3)
[ ${#A_KEYS[@]} -gt 0 ] && add_choice unitA "MacDanial Hills A (single unit)" "Model A on its own, AU and VST3" "${A_KEYS[@]}"
[ ${#B_KEYS[@]} -gt 0 ] && add_choice unitB "MacDanial Hills B (single unit)" "Model B (Class-A) on its own, AU and VST3" "${B_KEYS[@]}"

[ -n "$OUTLINE" ] || { echo "Nothing to package in $STAGE"; exit 1; }

mkdir -p "$WORK/resources"
cp "$HERE/welcome.html" "$WORK/resources/welcome.html"

cat > "$WORK/distribution.xml" <<XML
<?xml version="1.0" encoding="utf-8"?>
<installer-gui-script minSpecVersion="2">
    <title>${MAIN} ${VERSION}</title>
    <organization>${ID}</organization>
    <welcome file="welcome.html" mime-type="text/html"/>
    <options customize="allow" require-scripts="false" hostArchitectures="arm64,x86_64"/>
    <domains enable_localSystem="true"/>
    <choices-outline>
${OUTLINE}    </choices-outline>
${CHOICES}${REFS}</installer-gui-script>
XML

mkdir -p "$(dirname "$OUT")"
productbuild --distribution "$WORK/distribution.xml" --resources "$WORK/resources" --package-path "$WORK" "$OUT"

if [ -n "${INSTALLER_SIGN_ID:-}" ]; then
    productsign --sign "$INSTALLER_SIGN_ID" "$OUT" "$OUT.signed" && mv "$OUT.signed" "$OUT"
fi
echo "Built: $OUT"
echo "Contents:"; grep -o 'title="[^"]*"' "$WORK/distribution.xml" | sed 's/title=/  - /'
