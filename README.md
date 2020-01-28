# libcan

This interface simplifies CAN bus communication on a range of Microchip's 8-bit AVR microcontrollers.

## Operation

...

## Usage

...

## Build

The _tools_ folder contains an ATMEL Studio project that may be used to build program located in the _tests_ folder. The project is setup to link directy to the files in _inc_ and _src_, thus avoiding to make unnecessary copies. Its build-in compiler includes _inc_ as well.

A _makefile_ is provided, but is not setup to generate a programmable image. By using _make_, however, one may easily check whether the program compiles correctly and without warnings.

## Devices

Device | Status
--- | ---
AT90CAN128 | SUPPORTED
ATMEGA32M1 | UNTESTED