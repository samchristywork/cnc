import serial
import time

s = serial.Serial('/dev/ttyUSB0', 115200)

s.write(b"\r\n\r\n")
time.sleep(2)
s.flushInput()

s.write(b'M3 S100\n')
out = s.readline()
print(out)

time.sleep(1)

s.write(b'M3 S0\n')
out = s.readline()
print(out)

time.sleep(1)

s.write(b'$\n')
out = s.readline()
print(out)

time.sleep(2)
s.close()
