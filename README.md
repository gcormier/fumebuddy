<img src="https://github.com/gcormier/fumebuddy/blob/master/docs/board-3d.png?raw=true" height="200px" /> <img src="https://github.com/gcormier/fumebuddy/blob/master/docs/back-1.png?raw=true" height="200px" /> <img src="https://github.com/gcormier/fumebuddy/blob/master/docs/back-2.png?raw=true" height="200px" />

# FumeBuddy
FumeBuddy is an ESP32 replacement for the switch housing on the Hakko FX-950, FX-951 and FX-952. Out of the box, it was designed to toggle a fume extractor via a smart switch flashed with [Tasmota](https://tasmota.github.io/docs/#/). It is powered via USB, which allows it to be powered by a battery pack if desired.

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
### Tasmota
- Please visit the [Tasmota documentation](https://tasmota.github.io/docs/#/Commands?id=with-web-requests) for reference
- You need to get your smart plug working independantly of FumeBuddy. No support is provided for this - tasmota has an active [Discord community](https://discord.gg/Ks2Kzd4) if you need help!
- A static IP address or host name is needed
- If you are at the point that these commands properly toggle your switch, you're good to go!
````
http://<ip>/cm?cmnd=Power%20TOGGLE
http://<ip>/cm?cmnd=Power%20On
http://<ip>/cm?cmnd=Power%20off
````

#### Other Devices
- Ultimately, your goal is to have 3 URL's available which will control your smart device. Anything which can achieve this is controllable by FumeBuddy

### FumeBuddy
- FumeBuddy is pre-flashed with software which uses [Autoconnect](https://hieromon.github.io/AutoConnect/index.html)
- After powering up FumeBuddy, wait 30-60 seconds. You should find a new WiFi SSID (esp32ap) to which you can connect (password 12345678)

<img src="https://github.com/gcormier/fumebuddy/blob/master/docs/esp32ap.png?raw=true" height="200px" />

- You should receive a prompt that your WiFi network needs sign-in.

<img src="https://github.com/gcormier/fumebuddy/blob/master/docs/esp32ap-signin.png?raw=true" height="200px" />

- You can now configure your WiFi settings, and optionally FumeBuddy settings, although the second part is easier done from your desktop if you've done this on your mobile.

<img src="https://github.com/gcormier/fumebuddy/blob/master/docs/esp32-setting.png?raw=true" height="200px" />
<img src="https://github.com/gcormier/fumebuddy/blob/master/docs/tasmota_1.png?raw=true" height="200px" />
<img src="https://github.com/gcormier/fumebuddy/blob/master/docs/tasmota_2.png?raw=true" height="200px" />

- If there are issues connecting to WiFi, FumeBuddy will go back to hosting it's own WiFi network to repeat the above steps


## Operation
- If FumeBuddy is NOT powered, the iron will work as it did before with sleep function
- If FumeBuddy is powered via the 5V USB port, it will work based on what is programmed. By default, it will turn the plug ON when the iron is lifted. It will turn the plug OFF 5 seconds after the iron is put back
- Manual on/off can be by touching the capacitive wire on top of the housing for 1 second

## Pricing
The BOM cost alone for the product is around 23USD, which is rather expensive. I order **genuine** parts from 1st party distributors in Canada, which in this case includes Omron switches, solid state relays and the ESP32. I did not go cheap on this as owners of Hakko's are likely looking for quality components throughout. The rest includes my labor costs to assembly, flash and test the unit. As it is open sourced, you are welcome to build your own! You can cheap out on some components if you'd like. Knockoff relays and omitting the audio buzzer will save you a few dollars.

## Lead Time
As the BOM costs are quite high, I will be keeping limited number of parts on hand, and will assemble them as when ordered.

# etc
## TASMOTA Config
Turn off MQTT if not needed. This will allow the status LED to work based on the on/off status of the relay. "SetOption3 0" in the tasmota console.

# FAQ
##







