# ESP32 - example of FUOTA with Rollback

## Overview

This repository contains an example on how to do a Firmware Update Over-The-Air (FUOTA) in ESP32, also adding rollback feature to this process.
Then, in case of updating a bad firmware to ESP32, it'll be able to automatically get back to last stable firmware version flashed in it, making ESP32 able to recover itself from a bad firmware update.

This example is used in "Making robust FUOTA in ESP32 using Rollback" presentation in Embedded Online Conference 2023: https://embeddedonlineconference.com/session/Making_robust_FUOTA_in_ESP32_using_Rollback

## Important notes

Here are some important notes regarding this example:

* This example has been developed using ESP-IDF v4.4
* ESP32 received the firmware via TCP socket in port 5000. This example makes no distinguish between any kind of data received, considering everything piece of data received as a firmware chunk.
* After receiving at least one data chunk, if nothing was received in the lst 5 seconds, ESP32 considers firmware transmission has ended, and tries to update firmware.
* This repository contains 2 firmware binaries for testing rollback mechanism: good firmware (V1.2) and bad firmware (V1.3).
