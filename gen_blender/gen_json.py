import json
import random

brightness = 150
frames = [
    [random.randint(1, 255), random.randint(1, 255), random.randint(1, 255), brightness] for _ in range(100)
]

TIME = 100
FADE = True
LED_LEN = 20

# control.json
control = {"fps": 30, "OFPARTS": {}, "LEDPARTS": {}}

for i in range(10):
    control["OFPARTS"][f"OF{i}"] = i

for i in range(2):
    control["LEDPARTS"][f"LED{i}"] = {"id": i, "len": LED_LEN}

with open("control.json", "w") as f:
    json.dump(control, f, indent=2)
    print("control.json generated")

# OF.json
OF = []

for i, frame in enumerate(frames):
    status = {}
    for j in range(10):
        status[f"OF{j}"] = frame
    OF.append({"start": i * TIME, "fade": FADE, "status": status})

with open("OF.json", "w") as f:
    json.dump(OF, f, indent=2)
    print("OF.json generated")

# LED.json
LED = {}
for i in range(2):
    LED[f"LED{i}"] = []
    for j, frame in enumerate(frames):
        status = []
        for _ in range(LED_LEN):
            status.append(frame)
        LED[f"LED{i}"].append({"start": j * TIME, "fade": FADE, "status": status})

with open("LED.json", "w") as f:
    json.dump(LED, f, indent=2)
    print("LED.json generated")