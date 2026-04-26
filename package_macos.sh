#!/usr/bin/env bash
# ----------------------------------------------------------------------
# package_macos.sh — build & bundle a distributable .app for itch.io.
#
# Usage:
#   chmod +x package_macos.sh
#   ./package_macos.sh
#
# Output:
#   dist/TrenchDefense.app          <- the runnable .app bundle
#   dist/TrenchDefense-mac.zip      <- the file to upload to itch.io
# ----------------------------------------------------------------------
set -euo pipefail

APP_NAME="DoomDefense"
DISPLAY_NAME="Doom Defense"
BUNDLE_ID="com.armandgago.doomdefense"
VERSION="1.0.0"

PROJECT_ROOT="$(cd "$(dirname "$0")" && pwd)"
BUILD_DIR="$PROJECT_ROOT/build_release"
DIST_DIR="$PROJECT_ROOT/dist"
APP_DIR="$DIST_DIR/$APP_NAME.app"

# ------- 1. Clean -------
echo "==> Cleaning previous output"
rm -rf "$BUILD_DIR" "$APP_DIR" "$DIST_DIR/$APP_NAME-mac.zip"
mkdir -p "$BUILD_DIR" "$DIST_DIR"

# ------- 2. Compile (universal arm64 + x86_64) -------
ARCHS=(-arch arm64 -arch x86_64)
DEPLOY_TARGET="-mmacosx-version-min=11.0"
OPT="-O3"
WARN="-Wno-deprecated-declarations -Wno-deprecated-builtins -Wno-everything"

INCLUDES=(
    -I"$PROJECT_ROOT"
    -I"$PROJECT_ROOT/third_party"
    -I"$PROJECT_ROOT/third_party/lua"
    -I"$PROJECT_ROOT/rapidjson-1.1.0/include"
    -I"$PROJECT_ROOT/box2d-2.4.1/include"
    -I"$PROJECT_ROOT/box2d-2.4.1/src"
    -I"$PROJECT_ROOT/SDL2"
    -I"$PROJECT_ROOT/SDL_image"
    -I"$PROJECT_ROOT/SDL_mixer"
    -I"$PROJECT_ROOT/SDL2_ttf"
    -F"$PROJECT_ROOT/SDL2/lib"
    -F"$PROJECT_ROOT/SDL_image/lib"
    -F"$PROJECT_ROOT/SDL_mixer/lib"
    -F"$PROJECT_ROOT/SDL2_ttf/lib"
)

# Collect sources
LUA_SRCS=( "$PROJECT_ROOT"/third_party/lua/*.c )
BOX2D_SRCS=(
    "$PROJECT_ROOT"/box2d-2.4.1/src/collision/*.cpp
    "$PROJECT_ROOT"/box2d-2.4.1/src/common/*.cpp
    "$PROJECT_ROOT"/box2d-2.4.1/src/dynamics/*.cpp
    "$PROJECT_ROOT"/box2d-2.4.1/src/rope/*.cpp
)
CPP_SRCS=( "$PROJECT_ROOT"/*.cpp )

echo "==> Compiling Lua (C)"
LUA_OBJS=()
for f in "${LUA_SRCS[@]}"; do
    obj="$BUILD_DIR/$(basename "$f").o"
    clang -c "$f" "${ARCHS[@]}" $DEPLOY_TARGET $OPT "${INCLUDES[@]}" $WARN -o "$obj"
    LUA_OBJS+=("$obj")
done

echo "==> Compiling Box2D (C++)"
BOX2D_OBJS=()
for f in "${BOX2D_SRCS[@]}"; do
    obj="$BUILD_DIR/$(basename "$f").o"
    clang++ -std=c++17 -c "$f" "${ARCHS[@]}" $DEPLOY_TARGET $OPT "${INCLUDES[@]}" $WARN -o "$obj"
    BOX2D_OBJS+=("$obj")
done

echo "==> Compiling engine (C++)"
CPP_OBJS=()
for f in "${CPP_SRCS[@]}"; do
    obj="$BUILD_DIR/$(basename "$f").o"
    clang++ -std=c++17 -c "$f" "${ARCHS[@]}" $DEPLOY_TARGET $OPT "${INCLUDES[@]}" $WARN -o "$obj"
    CPP_OBJS+=("$obj")
done

echo "==> Linking"
BINARY="$BUILD_DIR/game_engine"
clang++ -std=c++17 "${ARCHS[@]}" $DEPLOY_TARGET \
    "${CPP_OBJS[@]}" "${BOX2D_OBJS[@]}" "${LUA_OBJS[@]}" \
    -F"$PROJECT_ROOT/SDL2/lib" \
    -F"$PROJECT_ROOT/SDL_image/lib" \
    -F"$PROJECT_ROOT/SDL_mixer/lib" \
    -F"$PROJECT_ROOT/SDL2_ttf/lib" \
    -framework SDL2 -framework SDL2_image \
    -framework SDL2_mixer -framework SDL2_ttf \
    -framework Cocoa \
    -Wl,-rpath,@executable_path/../Frameworks \
    -o "$BINARY"

if [ ! -x "$BINARY" ]; then
    echo "ERROR: link failed"
    exit 1
fi

# ------- 3. App bundle -------
echo "==> Creating .app skeleton"
mkdir -p "$APP_DIR/Contents/MacOS"
mkdir -p "$APP_DIR/Contents/Frameworks"
mkdir -p "$APP_DIR/Contents/Resources"

echo "==> Copying engine binary + game resources"
cp "$BINARY" "$APP_DIR/Contents/MacOS/game_engine"
cp -R "$PROJECT_ROOT/resources" "$APP_DIR/Contents/MacOS/resources"

echo "==> Embedding SDL2 frameworks"
cp -R "$PROJECT_ROOT/SDL2/lib/SDL2.framework"            "$APP_DIR/Contents/Frameworks/"
cp -R "$PROJECT_ROOT/SDL_image/lib/SDL2_image.framework" "$APP_DIR/Contents/Frameworks/"
cp -R "$PROJECT_ROOT/SDL_mixer/lib/SDL2_mixer.framework" "$APP_DIR/Contents/Frameworks/"
cp -R "$PROJECT_ROOT/SDL2_ttf/lib/SDL2_ttf.framework"    "$APP_DIR/Contents/Frameworks/"

echo "==> Writing launcher (so resources/ is found regardless of cwd)"
LAUNCHER="$APP_DIR/Contents/MacOS/$APP_NAME"
cat > "$LAUNCHER" <<'EOF'
#!/bin/bash
DIR="$(cd "$(dirname "$0")" && pwd)"
cd "$DIR"
exec "./game_engine"
EOF
chmod +x "$LAUNCHER"
chmod +x "$APP_DIR/Contents/MacOS/game_engine"

echo "==> Generating .icns from title_screen.png (optional)"
ICON_SRC="$PROJECT_ROOT/resources/images/title_screen.png"
if [ -f "$ICON_SRC" ] && command -v iconutil >/dev/null 2>&1; then
    ICONSET="$BUILD_DIR/AppIcon.iconset"
    rm -rf "$ICONSET"; mkdir -p "$ICONSET"
    declare -a SIZES=(
        "16:icon_16x16.png"        "32:icon_16x16@2x.png"
        "32:icon_32x32.png"        "64:icon_32x32@2x.png"
        "128:icon_128x128.png"     "256:icon_128x128@2x.png"
        "256:icon_256x256.png"     "512:icon_256x256@2x.png"
        "512:icon_512x512.png"     "1024:icon_512x512@2x.png"
    )
    # iconutil requires RGBA PNGs; sips strips alpha, so use ffmpeg if available.
    for ENTRY in "${SIZES[@]}"; do
        SZ="${ENTRY%%:*}"
        NM="${ENTRY##*:}"
        if command -v ffmpeg >/dev/null 2>&1; then
            ffmpeg -y -loglevel error -i "$ICON_SRC" \
                -vf "scale=${SZ}:${SZ}:flags=lanczos,format=rgba" \
                "$ICONSET/$NM"
        else
            sips -z "$SZ" "$SZ" "$ICON_SRC" --out "$ICONSET/$NM" >/dev/null
        fi
    done
    iconutil -c icns "$ICONSET" -o "$APP_DIR/Contents/Resources/AppIcon.icns" 2>/dev/null || \
        echo "    (icon generation skipped — bundle will use generic icon, set one later via Get Info)"
fi

echo "==> Writing Info.plist"
cat > "$APP_DIR/Contents/Info.plist" <<EOF
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN"
  "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
    <key>CFBundleName</key>                <string>$DISPLAY_NAME</string>
    <key>CFBundleDisplayName</key>         <string>$DISPLAY_NAME</string>
    <key>CFBundleIdentifier</key>          <string>$BUNDLE_ID</string>
    <key>CFBundleVersion</key>             <string>$VERSION</string>
    <key>CFBundleShortVersionString</key>  <string>$VERSION</string>
    <key>CFBundleExecutable</key>          <string>$APP_NAME</string>
    <key>CFBundleIconFile</key>            <string>AppIcon</string>
    <key>CFBundlePackageType</key>         <string>APPL</string>
    <key>LSMinimumSystemVersion</key>      <string>11.0</string>
    <key>NSHighResolutionCapable</key>     <true/>
    <key>NSPrincipalClass</key>            <string>NSApplication</string>
</dict>
</plist>
EOF

echo "==> Ad-hoc codesign"
codesign --force --deep --sign - "$APP_DIR" 2>/dev/null || \
    echo "    (codesign skipped — fine; users right-click -> Open the first time)"

echo "==> Zipping for itch.io"
( cd "$DIST_DIR" && /usr/bin/ditto -c -k --sequesterRsrc --keepParent "$APP_NAME.app" "$APP_NAME-mac.zip" )

echo
echo "==============================================================="
echo "  Done."
echo "  App:  $APP_DIR"
echo "  Zip:  $DIST_DIR/$APP_NAME-mac.zip"
echo "  Test: open \"$APP_DIR\""
echo "==============================================================="
