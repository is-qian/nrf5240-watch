## nrf5340-peripherals test(evt)

### Description
#### a demo project include the code for uart, adc, gpio, qspi, spi, i2c, pwm, i2s...

### Test details
|peripherals|pin|test detail|
|-|-|-|
|LCD backlight|P1.07:enable<br>P1.08:pwm|Connect J4<br>P1.08(TP3) toggle every 1 second<br>P1.07(TP2) at voltage 0.9v|
|LCD spi|P0.21:CS<br>P0.19:CLK<br>P0.20:MOSI<br>P0.22:MISO|measuring waveform on TP35/TP36/TP37<br>direct connection TP37 and TP38 to test loop back<br> uart log:`tx (ii) : 0003 0004 0105 rx (ii) : 0003 0004 0105`|
|magnetic iic|P1.06:IIC2_SCL<BR>P1.05:IIC2_SDA|measuring waveform on TP40/TP41<BR>uart log:`I2C device found at address 0x14`|
|acc&gyro spi|P0.28:CS<BR>P0.29:CLK<BR>P0.30:MOSI<BR>P0.31:MISO|measuring waveform on TP45/TP44/TP43|
|acc&gyro int|P0.23:INT1<BR>P0.24:INT2|Before test, remove R66.<br>TP46/TP47 both toggle every 1 second|
|pressure|P0.27:CS|TP49 toggle every 1 second|
|pressure int|P0.26:INT|Before remove R54<br>TP48 toggle every 1 second|
|adc light sensor|P0.04:ADC input|The adc value output via uart log:`P0.04 adc: 342`|
|digital light sensor|P1.05:IIC2_SDA<BR>P1.06:IIC2_SCL|measuring waveform on TP51/TP52<BR>uart log:`I2C device found at address 0x44`|
|codec iic|P1.05:IIC2_SDA<BR>P1.06:IIC2_SCL|measuring waveform on TP14/TP15<BR>uart log:`I2C device found at address 0x1a`|
|buttons|P1.00<br>P0.05<br>P0.06<br>P0.07|Button value showed in uart log:`button1:1 button2:1 button3:1 button4:1`|
|pmic(nPM1300)|P1.03:IIC1_SCL<BR>P1.02:IIC1_SDA|button value showed in uart log:`Short press`|
|qspi|P0.18:cs<br>P0.17:CLK<BR>P0.13:DATA0<BR>P0.14:DATA1<BR>P0.15:DATA2<BR>P0.16:DATA3|measuring waveform on R36/R37/R38/R39/R40/R41|

