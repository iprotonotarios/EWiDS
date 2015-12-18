#/bin/bash

dev="$(ls /dev/ | grep tty.usb | head -n 1)"

echo "Upload to first USB device"
/Users/mcattani/Documents/nRF51_SDK_9/examples/EWiDS/nrf51_dfu_serial/./serial_dfu.py /Users/mcattani/Dropbox/tudelft/nemo/fusion/fusion_day/fusion_day2/bin/fusionDayTwoApp.bin -p /dev/$dev