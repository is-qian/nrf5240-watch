from pyftdi.i2c import I2cController, I2cNackError

# Instantiate an I2C controller
i2c = I2cController()

# Configure the first interface (IF/1) of the FTDI device as an I2C master
i2c.configure('ftdi://ftdi:4232:1:1d/1')

# Get a port to an I2C slave device
slave = i2c.get_port(0x40)

# reset the chip
slave.exchange([0x00, 0xc1, 0x27], 1)

# config the cali register for current reg: 3.2uA/bit, for power reg: 80uW/bit
slave.exchange([0x05, 0x0c, 0x80], 1)

# Read a register from the I2C slave
x = slave.read_from(0x01, 2)
#print("reg shuntvol(0x01):"+x.hex())
print("shunt voltage:", round(int.from_bytes(x, byteorder='big')*0.0025, 4),"mV")
x = slave.read_from(0x02, 2)
#print("reg busvol (0x02):"+x.hex())
print("bus voltage:",int.from_bytes(x, byteorder='big')*1.25,"mV")
x = slave.read_from(0x03, 2)
#print("reg power(0x03):"+x.hex())
print("power:", int.from_bytes(x, byteorder='big')*80/1000, "mW")
x = slave.read_from(0x04, 2)
#print("reg curr(0x04):"+x.hex())
print("current:", int.from_bytes(x, byteorder='big')*3.2/1000, "mA")
x = slave.read_from(0x05, 2)
print("reg cali(0x05):"+x.hex())
