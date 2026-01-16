import sys

if len(sys.argv) > 1:
    filename = sys.argv[1]
else:
    filename = input("Enter filename to read (default: Frame.dat): ")
    if filename == "":
        filename = "Frame.dat"

try:
    # Read and print bytes
    with open(filename, 'rb') as f:
        for i, byte in enumerate(f.read()):
            print(f"{byte}", end=' ')
    
    print(f"\n\nFile '{filename}' read successfully.")
    
except FileNotFoundError:
    print(f"Error: File '{filename}' not found.")
except Exception as e:
    print(f"Error reading file: {e}")