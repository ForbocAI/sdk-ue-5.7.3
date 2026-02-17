#!/bin/bash
set -e

UE_ROOT="/Users/Shared/Epic Games/UE_5.7"
UAT_PATH="$UE_ROOT/Engine/Build/BatchFiles/RunUAT.sh"
PROJECT_ROOT="/Users/seandinwiddie/GitHub/Forboc.AI/sdk-ue-5.7.3"
PLUGIN_Path="$PROJECT_ROOT/Plugins/ForbocAI_SDK/ForbocAI_SDK.uplugin"
OUTPUT_DIR="$PROJECT_ROOT/dist_ForbocAI_SDK"

echo "Building ForbocAI SDK Plugin..."
"$UAT_PATH" BuildPlugin -Plugin="$PLUGIN_Path" -Package="$OUTPUT_DIR" -Rocket

echo "Build complete. Output in $OUTPUT_DIR"
