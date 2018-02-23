# absniffer-ibeacon

nRF52-based configurable iBeacon for running on ABSniffer USB 528 Dongle

## Serial Command Interface

When plugged in to a USB port, the device exposes a virtual serial port, over which it
accepts commands. The serial port parameters are 115200/8-N-1.
Commands are in textual form and terminated by line breaks.

### Retrieve Device Information

The `I` command is used to retrieve the firmware version and MAC address of the device.

```
> I
< OK V1.0.0 4e:60:b8:9c:cb:ed
```

### Set iBeacon Configuration

The `C` command is used to set the advertising parameters for the iBeacon.
The proximity UUID (16 bytes) is specified as a 32 character hex string.
The major and minor values are given as integers.

```
> C AABBCCDDAABBCCDDAABBCCDDAABBCCDD 123 456
< OK
```