# ESP32WifiHacks
Easy to use WiFi tools for the ESP32. Graphical interface provided via IMGUI.

Created as a bit of an experiment, not entirely practical, but neat.

![image](https://user-images.githubusercontent.com/65134690/221854171-6e5a74c0-b164-4a0f-adb9-0bd12d759464.png)

# Platform support
Linux-only due to limitations of the ArduinoSerialIO library, but an update to that library should result in it working cross-platform just fine (maybe I should do that some time).

# Features
* Sniff wifi traffic (Data, beacons, deauth, see image about)
* Simple deauth against an AP (Currently will likely not work against almost any device)
* Spam beacon frames (Create many fake access points, see image below)
 <img src="https://user-images.githubusercontent.com/65134690/221861221-99e581dc-563b-472b-b25b-19201919e1b0.png" width="50%">

# Setup
## Using the ESP-IDF to flash the ESP32
* https://github.com/espressif/vscode-esp-idf-extension/blob/master/docs/tutorial/install.md
* Open the "espcode" folder
* Build the project and flash it to the ESP32

## Compiling the host program
Create a build directory:
* `cd HostCode`
* `mkdir build`
* `cd build`

Use CMake to compile:
* `cmake .. && cmake --build .`

## Using the host program
Run the host program, specifying the serial port to open.
For example: `./ESP32HacksInterface /dev/ttyUSB0`
