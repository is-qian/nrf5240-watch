### openocd compile
```
git clone http://openocd.zylin.com/openocd
cd openocd
./bootstrap
./configure --enable-ftdi
make
sudo make install
```

### openocd usage with nrf5340(need to be verified)
```
openocd -f ./nrf5340_evt_openocd.cfg -f ./tcl/target/nrf53.cfg -c "program test.hex verify reset exit"
```
