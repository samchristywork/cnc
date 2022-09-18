import serial
import time

s=serial.Serial("/dev/ttyUSB0", 115200)

def send_gcode(g):
    s.write(g+b"\r\n")

time.sleep(2)
#send_gcode(b"G28 X0 Y0 Z0") # Home all axes?
send_gcode(b"G21") # Millimeters
#send_gcode(b"G91") # Relative motion
send_gcode(b"G90") # Absolute motion
send_gcode(b"G92 X0 Y0 Z0") # Specify that the head is at (0,0,0)
send_gcode(b"G1 F20 X0 Y0 Z-5")
#send_gcode(b"M3 S0") # Set spindle speed. This is required for other speed operations
#send_gcode(b"G1 F20 X5 Y0 Z0 S10")
#send_gcode(b"M3 S0") # Set spindle speed to 0
#send_gcode(b"G0 X0 Y0 Z0") # Go home
s.close()
