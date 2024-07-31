# Enable pyserial extensions
import pyftdi.serialext

# Open a serial port on the second FTDI device interface (IF/2) @ 3Mbaud
port = pyftdi.serialext.serial_for_url('ftdi://ftdi:4232:FT9O7BIF/1', baudrate=115200)

# Send bytes
port.write(b'Hello World')

# Receive bytes
while True:
    line = port.readline()
    print(line)
