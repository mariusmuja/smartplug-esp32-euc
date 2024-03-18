# Smartplug EUC Charge Control

This is an esphome-based firmware for ESP32 smartplugs (or any other ESP32 controlled relays) to control the charging of EUCs, specifically to turn off charging when a certain voltage (state of charge) is reached to prevent fully charging, which is beneficial for battery longevity.

Something similar can be accomplished with the EUCWorld app, which monitors the EUC battery lever and commands a smartplug to turn off charging, however that requires a smartphone to be in the proximity of the EUC. This firmware loads onto the smartplug and talks directly with the EUC over bluetooth (BLE).

## Supported EUCs

This projects currently supports the following EUCs:

  * Veteran/Leaperkim
  * ... more to be added in the future

## How to use

Any ESP32-based smartplugs could be used with this firmware by adapting the provided `.yaml` file, however we recommend the SwitchBot Smart Plug Mini (W1901400/W1901401) with because (as of now) the default firmware them can be replaced without requiring dissasembly with the help of the SwitchbOTA project (instructions here: https://github.com/kendallgoto/switchbota).

Steps to flash this on a SwitchBot Smart Plug Mini:
  * follow the instructions at https://github.com/kendallgoto/switchbota to replace the default firmware with Tasmota firmware
  * Install esphome (`pip install esphome` if you have python installed)
  * create a file named `secrets.yaml` with the following content:
  ```
wifi_ssid: <your WiFi SSID name>
wifi_password: <your WiFi password>
euc_mac: <your EUC bluetooth MAC address>
  ```
  * uncomment the `webserver:` line if you're not planing to use Home Assistant (you can also comment out the `api:` section in that case)
  * compile the project using `esphome compile switchbot-plug-mini-esphome-euc.yml`. This will generate a `firmware.bin` file in the project sub-directory: `.esphome/build/switchbot-plug-euc/.pioenvs/switchbot-plug-euc/`
  * open the Tasmota web page by typing the smartplug IP address in the a browser, go to the firmware update option and flash the `firmware.bin` file created in the previous step
  * the smartplug should automatically connect to the EUC on reboot. Adopt the smartplug in Home Assistant or go to the smartplug web page to configure the charge stop&start voltage thresholds. Charging starts automatically if the EUC battery voltage is below the start threshold and stop when it gets above the stop threshold. You can manually start the smartplug or start it with a configurable delay.

