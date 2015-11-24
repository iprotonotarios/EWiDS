#/bin/bash

dev="$(ls /dev/ | grep tty.usb | head -n 1)"

echo "Make project"
make sniffer
echo "Upload to first USB device"
/Users/mcattani/Documents/nRF51_SDK_9/examples/EWiDS/nrf51_dfu_serial/./serial_dfu.py _build/nrf51422_xxac.bin -p /dev/$dev