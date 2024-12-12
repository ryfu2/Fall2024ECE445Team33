import serial
import time

ser = None
while ser is None:
    try:
        ser = serial.Serial('COM5', 115200)
    except serial.SerialException:
        time.sleep(1)  # wait for serial

with open("test_output.wav", "wb") as f: 
    print("receiving")
    index = 0
    while True:
        data = ser.read(1024) 
        # print(data)
        f.write(data)
        print(index)
        index = index + 1

# with open("test_output.wav", "rb") as f:
#     header = f.read(44)
#     print("WAV Header:", header)
