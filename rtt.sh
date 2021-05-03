#!/bin/sh
# script to start RTT Console via JLinkExe
#
xterm -title 'JLinkExe' -e bash -c 'JLinkExe  -autoconnect 1 -device NRF52832_XXAA -if SWD -speed 1000' &
sleep 1
xterm -title 'JLinkRTTClient' -e bash -c 'JLinkRTTClient' &
echo "**********************************************************************"
echo "* In JLinkExe shell, enter 'h [enter]', 'r [enter]', and 'g [enter]' *"
echo "* to reset and start the target device.                              *"
echo "**********************************************************************"
