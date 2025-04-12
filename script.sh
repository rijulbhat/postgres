#!/bin/bash

# Define the target directory
TARGET_DIR="submission"

# Create the target directory
mkdir -p "$TARGET_DIR"

# List of files
FILES=(
    "src/backend/access/common/printtup.c"
    "src/backend/executor/execMain.c"
    "src/backend/lib/Makefile"
    "src/backend/lib/meson.build"
    "src/backend/lib/my_vector.c"
    "src/backend/optimizer/plan/planner.c"
    "src/backend/parser/analyze.c"
    "src/backend/parser/gram.y"
    "src/include/access/printtup.h"
    "src/include/lib/my_vector.h"
    "src/include/nodes/parsenodes.h"
    "src/include/nodes/plannodes.h"
    "src/include/parser/kwlist.h"
    "src/include/tcop/dest.h"
    "src/tools/pgindent/typedefs.list"
)

# Copy files while preserving directory structure
for file in "${FILES[@]}"; do
    mkdir -p "$TARGET_DIR/$(dirname "$file")"
    cp "$file" "$TARGET_DIR/$file"
done