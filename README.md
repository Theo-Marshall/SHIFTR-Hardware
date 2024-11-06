# SHIFTR
A BLE to Direct Connect bridge for bike trainers using a WT32-ETH01 module based on ESP32. Additionally adding Zwift™️ "virtual shifting" functionality to any device supporting FE-C over BLE.

## Overview
Bike trainers heavily evolved over the last years. Tacx (acquired by Garmin some years ago) is one of the market leaders producing bike trainers of a very high quality with special features like e.g. "Road Feeling" to simulate different road surfaces. 
Other vendors like Zwift came up with cool new features like "virtual shifting" using the "Zwift Cog" and a controller device (Zwift Click/Play) to have a non-mechanical shifting solution which is setting the gears via software that adjusts the resistance in the trainer. 
Wahoo came up with a technology called "Direct Connect" that allows the trainer devices to be connected via Ethernet (or even WiFi) instead of bluetooth to have a more stable connection.

While other vendors like e.g. Elite or JetBlack are very fast in implementing things like virtual shifting to their newer models Garmin is very slow and unresponsive when it comes to such feature requests. Probably also because they have their own ecosystem that competes with Zwift. 

Using Zwift since many years and owning a "Tacx Vortex" and a "Tacx Neo 2T" bike trainer I have been jealous to features like virtual shifting and the possibitlity to connect the trainer device via ethernet. So the idea for this project, SHIFTR, was born by having a device between Zwift and the Tacx supporting all this cool new functionality.

After many days and nights of searching the internet for a specification of the protocols I was only able to find small parts for "Direct Connect" that has already been reverse engineered by Roberto Viola in his [QDomyos-Zwift](https://github.com/cagnulein/qdomyos-zwift) project. The code helped me a lot to understand the protocol.
Unfortunately there was absolutely no information about the "virtual shifting" to be found. So a very long session of decompiling Zwift apps and sniffing BLE traffic between Zwift and capable trainers (which fortunately a friend had one of) started. It took some weeks but I was able to reverse engineer the used protocols.

As I wanted to have a convenient solution that doesn't require a special program or app to be started to have that functionality I decided to implement everything on an ESP32 micro controler that already features WiFi and Bluetooth onboard and also optionally ethernet. The perfect module for that was the [WT32-ETH01](https://en.wireless-tag.com/product-item-2.html) module that comes with everything I needed. The device will be placed very close to the trainer in a 3D printed case that also has place for step down converter to be able to use the existing power supply from the trainer.

![The whole setup](images/tacx_neo_2t_and_zwift_cog_and_device.jpg)

## Needed hardware
- WT32-ETH01 ESP32 board (e.g. from [Amazon](https://www.amazon.de/WT32-ETH01-Embedded-Schnittstelle-Bluetooth-Entwicklungsplatine/dp/B0CW3DDWZ4))
- Step-Down converter 5-80V (the Tacx Neo supplies 48V) to 5V (e.g. from [Amazon](https://www.amazon.de/dp/B0CMC7Y3DJ))
- Y-Splitter for the Tacx power cable (e.g. from [Amazon](https://www.amazon.de/dp/B09FHHN9T5))
- Power connector for the case (e.g. from [Amazon](https://www.amazon.de/gp/product/B09PD6J4BN))
- 3D printed case that fits the WT32-ETH01. Luckily I was able to find a perfect match: [WT32-ETH01 Enclosure](https://www.thingiverse.com/thing:5621092)
- Two countersunk screws M2x7 to hold the lid on the case


## Hardware installation
- ***IMPORTANT***: Don't solder the pin headers that came with the WT32-ETH01. With the longer pins on the bottom side the module won't fit the case anymore.
- ***IMPORTANT #2***: Connect the step-down converter to a power supply (e.g. the original one with 48V) and a multimeter and use a screwdriver to adjust the output voltage to 5V before connecting it to the WT32-ETH01 module!
- Use a standard USB<->TTL converter (e.g. with FTDI232 or similar). Make sure that it can provide **3,3V and not 5V** and connect as follows (see also pinout above):
  | Converter | -> | ETH01-EVO | 
  |-|-|-|
  | RX | -> | TXD |
  | TX | -> | RXD |
  | 3V3 | -> | 3V3 |
  | GND | -> | GND |

  It's best to use some breadboard jumper wires that can just be connected without soldering. Use a rubber band to hold them in contact.

  To start the WT32-ETH01 in boot mode it is necessary to connect "IO0" with GND and then to reset the board shortly connect "EN" to GND for a quarter of a second.

## Software installation
- Make a copy of the provided ``ota.ini.example`` file and name it ``ota.ini`` (you can adjust the values in the file to your needs but also leave it as it is)
- Open the project in [PlatformIO](https://platformio.org) and let it install the dependencies
- Connect the programmer and upload via the task wt32-eth01 -> Upload and Monitor
- Connect an ethernet cable and make sure a DHCP server exists in your network
- After a few seconds you should be able to see the IP address and the hostname (like e.g. "SHIFTR-123456.local") in the monitor log
- If you don't use the Ethernet interface you can also use the WiFi functionality. For that the device opens an AP with the device name (e.g. "SHIFTR 123456") and the password is the same but with "-" insead of space. Please note that WiFi is provided by an external library called [IoTWebConf](https://github.com/prampec/IotWebConf) and more or less untested
- After that you can configure the device and its network settings in the web interface on e.g. http://SHIFTR-123456.local
- Further updates can be made over ethernet or WiFi so you can disconnect the programming adapter now

## Finalization
- Finally you can solder some wires to the power connector to the step down module and from there to the WT32-ETH01. Then mount everything in the 3D printed case:

  ![Mounted adapter](images/wt32-eth01_case.jpg)

- Use the Y-Power-Splitter to connect the Tacx trainer and the device:
  
  ![Connected to Tacx](images/connected_to_tacx_neo_2t.jpg)

- Power the Trainer and go to the web interface
- Go to "Settings", select the trainer device and "Save"
- Make sure the "Virtual shifting" option is enabled if you want to use it. Virtual shifting only works in Zwift when a Zwift Click or Zwift Play controllers are connected. Otherwise it will fall back to the normal ERG and SIM modes
- In Zwift you need to connect to the "SHIFTR 123456" device instead of your old device now. Have fun!

## Thank you!
You've made it to the end, hopefully you'll have fun rebuilding the whole thing. 
In case of any questions feel free to contact me!



