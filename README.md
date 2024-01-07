<img src="https://github.com/gcormier/fumebuddy/blob/master/docs/board-3d.png?raw=true" height="200px" /> <img src="https://github.com/gcormier/fumebuddy/blob/master/docs/back-1.png?raw=true" height="200px" /> <img src="https://github.com/gcormier/fumebuddy/blob/master/docs/back-2.png?raw=true" height="200px" />

# FumeBuddy
FumeBuddy is an ESP32 replacement for the switch housing on the Hakko FX-950, FX-951 and FX-952. Out of the box, it was designed to toggle a fume extractor via a smart switch flashed with [Tasmota](https://tasmota.github.io/docs/#/). It is powered via USB, which allows it to be powered by a battery pack if desired.

<a href="https://www.tindie.com/products/gcormier/fumebuddy"><img src="https://d2ss6ovg47m0r5.cloudfront.net/badges/tindie-mediums.png" alt="I sell on Tindie" width="150" height="78"></a>


## Design Goals
- Easily hacked
- Replace stock housing
- Default behavior when un-powered

## Video
[See FumeBuddy explained and in action](https://www.youtube.com/watch?v=eo5WyHWy1So)

## Hardware Installation
- Remove the old housing
- Transfer the spring from the old housing to the new housing
- Install using the old screws
- If you'd like the manual capacitive touch function and did not order it, you can solder a single wire to the appropriate jumper

## Configuration
FumeBuddy now uses [esphome](https://esphome.io/index.html). You don't need Home Assistant or any sort of smart home. It merely eliminates the need for me to do any custom coding.

<img src="https://github.com/gcormier/fumebuddy/blob/master/docs/esphome_portal.png?raw=true" height="800px" />

When you plug in the device, it will create it's own WiFi access point named `FumebuddyFallback` (Password is `fumebuddy`). You
can connect to this and it will redirect you to a captive portal where you can enter your WiFi credentials. Once you do this, it will reboot and connect to your WiFi. You can then access it via the IP address it was assigned by your router.

Accessing it via the IP address will give you a web interface where you can configure the device. Notably the 2 URL's that should be called when the iron is lifted and put back down.

The touch sensor threshold can be adjusted. The default is 250, but you can enable the sensitivity debug mode and view the output
in the web console to see which value works best for you.

## Operation
- If FumeBuddy is NOT powered, the iron will work as it did before with sleep function
- If FumeBuddy is powered via the 5V USB port, it will work based on what is programmed. By default, it will turn the plug ON when the iron is lifted. It will turn the plug OFF 5 seconds after the iron is put back
- Manual on/off can be by touching and holding the capacitive wire on top of the housing. It will keep the fan on until you touch it again.

## Pricing
The BOM cost alone for the product is around 23USD, which is rather expensive. I order **genuine** parts from 1st party distributors in Canada, which in this case includes Omron switches, solid state relays and the ESP32. I did not go cheap on this as owners of Hakko's are likely looking for quality components throughout. The rest includes my labor costs to assembly, flash and test the unit. As it is open sourced, you are welcome to build your own! You can cheap out on some components if you'd like. Knockoff relays and omitting the audio buzzer will save you a few dollars.
