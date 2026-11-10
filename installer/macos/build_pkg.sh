#!/bin/bash
# Builds a macOS installer (.pkg) that installs the AU and VST3 system-wide.
# Usage: build_pkg.sh "<path/to/Product.component>" "<path/to/Product.vst3>" <version> "<output.pkg>"
set -euo pipefail

COMPONENT="$1"; VST3="$2"; VERSION="$3"; OUT="$4"
NAME="$(basename "$COMPONENT" .component)"
HERE="$(cd "$(dirname "$0")" && pwd)"
WORK="$(mktemp -d)"
ID_BASE="com.basemental.macdanialhills"

stage_and_build () {   # $1 = bundle, $2 = install dir, $3 = package id, $4 = output component pkg
    local root="$WORK/root-$(basename "$4" .pkg)"
    mkdir -p "$root"
    cp -R "$1" "$root/"
    # Never let the installer "relocate" the bundle into some other copy found on disk.
    pkgbuild --analyze --root "$root" "$WORK/comp.plist"
    local i=0
    while /usr/libexec/PlistBuddy -c "Print :$i" "$WORK/comp.plist" >/dev/null 2>&1; do
        /usr/libexec/PlistBuddy -c "Set :$i:BundleIsRelocatable false" "$WORK/comp.plist"
        i=$((i + 1))
    done
    pkgbuild --root "$root" --component-plist "$WORK/comp.plist" \
             --identifier "$3" --version "$VERSION" --install-location "$2" "$4"
}

stage_and_build "$COMPONENT" "/Library/Audio/Plug-Ins/Components" "$ID_BASE.au"   "$WORK/au.pkg"
stage_and_build "$VST3"      "/Library/Audio/Plug-Ins/VST3"       "$ID_BASE.vst3" "$WORK/vst3.pkg"

mkdir -p "$WORK/resources"
cp "$HERE/welcome.html" "$WORK/resources/welcome.html"

cat > "$WORK/distribution.xml" <<XML
<?xml version="1.0" encoding="utf-8"?>
<installer-gui-script minSpecVersion="2">
    <title>${NAME}</title>
    <organization>${ID_BASE}</organization>
    <welcome file="welcome.html" mime-type="text/html"/>
    <options customize="allow" require-scripts="false" hostArchitectures="arm64,x86_64"/>
    <domains enable_localSystem="true"/>
    <choices-outline>
        <line choice="au"/>
        <line choice="vst3"/>
    </choices-outline>
    <choice id="au" title="Audio Unit (Logic Pro, GarageBand)" description="Installs to /Library/Audio/Plug-Ins/Components">
        <pkg-ref id="${ID_BASE}.au"/>
    </choice>
    <choice id="vst3" title="VST3 (Ableton Live, Cubase, Studio One, ...)" description="Installs to /Library/Audio/Plug-Ins/VST3">
        <pkg-ref id="${ID_BASE}.vst3"/>
    </choice>
    <pkg-ref id="${ID_BASE}.au"   version="${VERSION}">au.pkg</pkg-ref>
    <pkg-ref id="${ID_BASE}.vst3" version="${VERSION}">vst3.pkg</pkg-ref>
</installer-gui-script>
XML

productbuild --distribution "$WORK/distribution.xml" --resources "$WORK/resources" \
             --package-path "$WORK" "$OUT"

# Optional: sign the installer if a Developer ID Installer identity is available.
if [ -n "${INSTALLER_SIGN_ID:-}" ]; then
    productsign --sign "$INSTALLER_SIGN_ID" "$OUT" "$OUT.signed" && mv "$OUT.signed" "$OUT"
fi

echo "Built: $OUT"
