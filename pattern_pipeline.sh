#!/bin/bash

# LED Pattern Generator Pipeline Script
# This script automates the generation of frame.bin from JSON files
# Usage: ./run_pipeline.sh <input_dir> <output_dir>

# Check for correct number of arguments
if [ "$#" -ne 2 ]; then
    echo "Usage: $0 <input_dir> <output_dir>"
    echo "  input_dir  : Directory containing control.json, of.json, and led.json"
    echo "  output_dir : Directory for output files (control.bin, OF.txt, LED.txt, frame.bin)"
    exit 1
fi

# Set directories from command line arguments
INPUT_DIR="$1"
OUTPUT_DIR="$2"
SRC_DIR="src"

# Validate input directory
if [ ! -d "$INPUT_DIR" ]; then
    echo "Error: Input directory '$INPUT_DIR' does not exist"
    exit 1
fi

# Create output directory if it doesn't exist
mkdir -p "$OUTPUT_DIR"

# Define required input files
CONTROL_JSON="$INPUT_DIR/control.json"
OF_JSON="$INPUT_DIR/of.json"
LED_JSON="$INPUT_DIR/led.json"

echo "=== LED Pattern Generator Pipeline ==="
echo "Input directory : $INPUT_DIR"
echo "Output directory: $OUTPUT_DIR"
echo ""

# Function to check if a file exists
check_file() {
    if [ ! -f "$1" ]; then
        echo "Error: Required file '$1' not found"
        return 1
    fi
    return 0
}

# Step 1: Compile all C programs
echo "Step 1: Compiling C programs..."
echo "------------------------------"

# Compile read_from_control.c
echo "- Compiling read_from_control.c..."
gcc -o "$SRC_DIR/read_control" "$SRC_DIR/read_from_control.c"
if [ $? -ne 0 ]; then
    echo "Error: Failed to compile read_from_control.c"
    exit 1
fi

# Compile read_from_OF.c
echo "- Compiling read_from_OF.c..."
gcc -o "$SRC_DIR/read_OF" "$SRC_DIR/read_from_OF.c"
if [ $? -ne 0 ]; then
    echo "Error: Failed to compile read_from_OF.c"
    exit 1
fi

# Compile read_from_LED.c
echo "- Compiling read_from_LED.c..."
gcc -o "$SRC_DIR/read_LED" "$SRC_DIR/read_from_LED.c"
if [ $? -ne 0 ]; then
    echo "Error: Failed to compile read_from_LED.c"
    exit 1
fi

# Compile merge_frame.c
echo "- Compiling merge_frame.c..."
gcc -o "$SRC_DIR/merge_frame" "$SRC_DIR/merge_frame.c"
if [ $? -ne 0 ]; then
    echo "Error: Failed to compile merge_frame.c"
    exit 1
fi

echo "✓ All C programs compiled successfully"
echo ""

# Step 2: Generate control.bin from control.json
echo "Step 2: Generating control.bin from control.json"
echo "------------------------------------------------"

# Check if control.json exists
check_file "$CONTROL_JSON" || exit 1

# Run read_control program
echo "- Processing $CONTROL_JSON..."
"$SRC_DIR/read_control" "$CONTROL_JSON" "$OUTPUT_DIR/control.bin"

if [ $? -eq 0 ] && [ -f "$OUTPUT_DIR/control.bin" ]; then
    echo "✓ control.bin generated successfully"
    
    # Display control.bin content
    echo "  Control.bin content:"
    echo -n "    fps     : "
    od -An -t u1 -j 0 -N 1 "$OUTPUT_DIR/control.bin" | tr -d ' '
    echo -n "    OF_num  : "
    od -An -t u1 -j 1 -N 1 "$OUTPUT_DIR/control.bin" | tr -d ' '
    echo -n "    LED_num : "
    od -An -t u1 -j 2 -N 1 "$OUTPUT_DIR/control.bin" | tr -d ' '
    
    # Read LED bulb counts
    LED_NUM=$(od -An -t u1 -j 2 -N 1 "$OUTPUT_DIR/control.bin" | tr -d ' ')
    if [ "$LED_NUM" -gt 0 ]; then
        echo -n "    LED_bulbs: "
        od -An -t u1 -j 3 -N "$LED_NUM" "$OUTPUT_DIR/control.bin" | tr -d '\n'
        echo ""
    fi
else
    echo "✗ Failed to generate control.bin"
    exit 1
fi

echo ""

# Step 3: Generate OF.txt from of.json
echo "Step 3: Generating OF.txt from of.json"
echo "--------------------------------------"

# Check if of.json exists
check_file "$OF_JSON" || exit 1

# Run read_OF program
echo "- Processing $OF_JSON..."
"$SRC_DIR/read_OF" "$OF_JSON" "$OUTPUT_DIR/control.bin" "$OUTPUT_DIR/OF.txt"

if [ $? -eq 0 ] && [ -f "$OUTPUT_DIR/OF.txt" ]; then
    echo "✓ OF.txt generated successfully"
    
    # Display first few lines of OF.txt
    echo "  OF.txt preview (first 3 frames):"
    echo "  -------------------------------"
    grep -A 5 "^frame " "$OUTPUT_DIR/OF.txt" | head -15
else
    echo "✗ Failed to generate OF.txt"
    exit 1
fi

echo ""

# Step 4: Generate LED.txt from led.json
echo "Step 4: Generating LED.txt from led.json"
echo "----------------------------------------"

# Check if led.json exists
check_file "$LED_JSON" || exit 1

# Run read_LED program
echo "- Processing $LED_JSON..."
"$SRC_DIR/read_LED" "$LED_JSON" "$OUTPUT_DIR/control.bin" "$OUTPUT_DIR/LED.txt"

if [ $? -eq 0 ] && [ -f "$OUTPUT_DIR/LED.txt" ]; then
    echo "✓ LED.txt generated successfully"
    
    # Display first few lines of LED.txt
    echo "  LED.txt preview (first frame):"
    echo "  -----------------------------"
    grep -A 5 "^frame " "$OUTPUT_DIR/LED.txt" | head -10
else
    echo "✗ Failed to generate LED.txt"
    exit 1
fi

echo ""

# Step 5: Generate frame.bin by merging OF.txt and LED.txt
echo "Step 5: Generating frame.bin"
echo "----------------------------"

# Run merge_frame program
echo "- Merging OF.txt and LED.txt..."
"$SRC_DIR/merge_frame" "$OUTPUT_DIR/OF.txt" "$OUTPUT_DIR/LED.txt" "$OUTPUT_DIR/control.bin" "$OUTPUT_DIR/frame.bin"

if [ $? -eq 0 ] && [ -f "$OUTPUT_DIR/frame.bin" ]; then
    echo "✓ frame.bin generated successfully"
    
    # Display frame.bin file info
    FRAME_SIZE=$(stat -f%z "$OUTPUT_DIR/frame.bin" 2>/dev/null || stat -c%s "$OUTPUT_DIR/frame.bin")
    echo "  frame.bin size: $FRAME_SIZE bytes"
else
    echo "✗ Failed to generate frame.bin"
    exit 1
fi

echo ""

# Step 6: Display summary
echo "Step 6: Pipeline Summary"
echo "------------------------"

echo "✓ Pipeline completed successfully!"
echo ""
echo "Generated files in $OUTPUT_DIR/:"
echo "--------------------------------"
ls -lh "$OUTPUT_DIR"/*.bin "$OUTPUT_DIR"/*.txt 2>/dev/null | while read line; do
    echo "  $line"
done

echo ""
echo "File sizes:"
echo "-----------"
for file in "$OUTPUT_DIR"/control.bin "$OUTPUT_DIR"/OF.txt "$OUTPUT_DIR"/LED.txt "$OUTPUT_DIR"/frame.bin; do
    if [ -f "$file" ]; then
        size=$(stat -f%z "$file" 2>/dev/null || stat -c%s "$file")
        filename=$(basename "$file")
        printf "  %-12s: %d bytes\n" "$filename" "$size"
    fi
done

echo ""
echo "=== Pipeline Execution Complete ==="

# Optional: Clean up compiled executables
echo ""
echo "Cleaning up compiled executables..."
rm -f "$SRC_DIR/read_control" "$SRC_DIR/read_OF" "$SRC_DIR/read_LED" "$SRC_DIR/merge_frame"
echo "Cleanup completed."

exit 0