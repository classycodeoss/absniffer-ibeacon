# absniffer-ibeacon

nRF52-based configurable iBeacon for running on ABSniffer USB 528 Dongle

## Build Instructions

This guide assumes you have already have the nRF SDK (version 14.2.0), the command-line tools
and an ARM GCC toolchain installed on your machine. It's best to verify this by building one
of the SDK samples first.

Create a file called `CMakeEnv.cmake` and add the paths to those tools. Example:

```
set(ARM_NONE_EABI_TOOLCHAIN_PATH "/Users/ahs/local/gcc-arm-none-eabi-7-2017-q4-major")
set(NRF5_SDK_PATH "/Users/ahs/local/nRF5_SDK_14.2.0_17b948a")
set(NRFJPROG "/Users/ahs/local/nRF5x-Command-Line-Tools_9_7_2_OSX/nrfjprog/nrfjprog")
```


Create the Makefiles, erase the flash.flash of SoftDevice 
```
$ cmake -H. -B"build"
$ cmake --build build --target FLASH_ERASE
$ cmake --build build --target FLASH_SOFTDEVICE
```

Flash the SoftDevice (nRF BLE stack):

```
$ cmake --build build --target FLASH_SOFTDEVICE
```


Build and flash beacon firmware:
```
$ cmake --build build --target absniffer-ibeacon
$ cmake --build build --target FLASH_absniffer-ibeacon

```

## Serial Command Interface

When plugged in to a USB port, the device exposes a virtual serial port, over which it
accepts commands. The serial port parameters are 115200/8-N-1.
Commands are in textual form and terminated by line breaks.

### Retrieve Device Information

The `I` command is used to retrieve the firmware version, MAC address and beacon identity
of the device.

```
> I
< OK V1.0.0 ED:CB:9C:B8:60:4E CCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCC 1 1
```

### Set iBeacon Configuration

The `C` command is used to set the advertising parameters for the iBeacon.
The proximity UUID (16 bytes) is specified as a 32 character hex string.
The major and minor values are given as integers.

```
> C AABBCCDDAABBCCDDAABBCCDDAABBCCDD 123 456
< OK
```

The configuration is stored persisently in flash memory.
