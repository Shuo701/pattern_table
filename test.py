import sys

with open(sys.argv[1], 'rb') if len(sys.argv) > 1 else open('data.bin', 'rb') as f:
    for i, byte in enumerate(f.read()):
        print(f"{byte}", end=' ')