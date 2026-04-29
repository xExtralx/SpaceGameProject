#!/bin/bash

if [ -z "$1" ]; then
    echo "Usage: $0 <image.webp> [output.png]"
    exit 1
fi

INPUT="$1"
OUTPUT="${2:-${INPUT%.webp}.png}"

if [ ! -f "$INPUT" ]; then
    echo "ERROR: file not found: $INPUT"
    exit 1
fi

if ! command -v dwebp &> /dev/null; then
    echo "dwebp not found, installing..."
    sudo apt install -y webp
fi

dwebp "$INPUT" -o "$OUTPUT"
echo "Done: $OUTPUT"