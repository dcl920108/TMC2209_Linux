#!/usr/bin/env python3
"""Raw UART test with correctly computed CRC."""
import serial
import time

PORT = "/dev/ttyAMA2"
BAUD = 115200

def crc8_atm(data):
    crc = 0
    for byte in data:
        b = byte
        for _ in range(8):
            if (crc >> 7) ^ (b & 0x01):
                crc = ((crc << 1) ^ 0x07) & 0xFF
            else:
                crc = (crc << 1) & 0xFF
            b >>= 1
    return crc

# Read IFCNT (reg 0x02), slave 0x00
frame = [0x05, 0x00, 0x02]
frame.append(crc8_atm(frame))
packet = bytes(frame)

print(f"Frame: {packet.hex()}  (CRC computed = 0x{frame[-1]:02X})")

ser = serial.Serial(PORT, BAUD, timeout=0.1)
time.sleep(0.1)
ser.reset_input_buffer()

ser.write(packet)
ser.flush()
time.sleep(0.05)

response = ser.read(64)
print(f"Received {len(response)} bytes: {response.hex()}")

if len(response) >= 12:
    print(f"  Echo:  {response[:4].hex()}")
    print(f"  Reply: {response[4:12].hex()}")
    print("SUCCESS")
elif len(response) == 4:
    print("!! Only echo - TMC still not responding")
else:
    print(f"!! Unexpected: {len(response)} bytes")

ser.close()
