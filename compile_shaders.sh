#!/usr/bin/env bash

mkdir -p build/shaders

for file in shaders/*; do
    [ -f "$file" ] || continue

    filename="$(basename "$file")"

    glslc \
        "$file" \
        -o "build/shaders/$filename.spv"
done