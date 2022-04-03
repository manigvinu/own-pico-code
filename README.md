# own-pico-code

# Overview

This repo contains the implementation of pico code developed by Manikandan to address any bugs or new findings in order to explore the Rpi PICO board.


# DHT
The current implementation of dht11 sensor from mainline pico sdk is not working (C implementation ). The communication path between PICO board and dht11 sensor via GPIO is not working as it misses the key element of synchronization. TO overcome this issue the code present here contains the right implementation of getting the data from dht11 sensor as mentioned in the data sheet and converting the pulses into readable data to print out the right Temperature and Humidity values reported by the sensor.

## How to Build & Run
The dht directtory has to be copied over to the rpi SDK examples repo at the exact path where dht folder is present. The code contains the preprocessor switch to ORIGINAL_CODE which runs the aforedeveloped rpi pico code for dht11 which is buggy code.

Commenting out this macro will enable the working implementation which was developed by Manikandan. Build and Flash the binary stays the same without modifications and the wiring connections as well which is definitely a pre-requisite before running the code/copy the binary onto the board.

