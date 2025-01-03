from pyftdi.i2c import I2cController, I2cNackError
from pyftdi.ftdi import Ftdi
from pyftdi.misc import add_custom_devices
import re
import io
import sys

if len(sys.argv) != 2:
    print("Usage: ft4232hq_get_power_consume.py <id_hex>")
    sys.exit(1)

slave_id = int(sys.argv[1], 16)

# Get dev name of ft4232
add_custom_devices(Ftdi, force_hex=True)

output = io.StringIO()
sys.stdout = output

Ftdi.show_devices()

sys.stdout = sys.__stdout__

devlist = output.getvalue()

match = re.search(r'ftdi://ftdi:4232:1:[a-f0-9]+/1', devlist) 

if match:
    dev_str = match.group(0)
    print(dev_str)
else:
    print("No device found")

# Instantiate an I2C controller
i2c = I2cController()

# Configure the first interface (IF/1) of the FTDI device as an I2C master
i2c.configure(dev_str)

# Get a port to an I2C slave device
slave = i2c.get_port(slave_id)

# reset the chip
slave.exchange([0x00, 0xc1, 0x27], 1)

# config the cali register for current reg: 3.2uA/bit, for power reg: 80uW/bit
slave.exchange([0x05, 0x0c, 0x80], 1)

# Read a register from the I2C slave
x = slave.read_from(0x01, 2)
print("shunt voltage:", round(int.from_bytes(x, byteorder='big')*0.0025, 4),"mV")

x = slave.read_from(0x02, 2)
print("bus voltage:",int.from_bytes(x, byteorder='big')*1.25,"mV")

x = slave.read_from(0x03, 2)
print("power:", int.from_bytes(x, byteorder='big')*80/1000, "mW")

x = slave.read_from(0x04, 2)
print("current:", round(int.from_bytes(x, byteorder='big')*3.2/1000, 4), "mA")

x = slave.read_from(0x05, 2)
print("reg cali(0x05):"+x.hex())
