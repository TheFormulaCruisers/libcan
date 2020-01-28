# libcan

This interface simplifies CAN bus communication on a range of Microchip's 8-bit AVR microcontrollers.

## Operation

...

## Usage

Functions to this interface are defined in the corresponding header files in _inc_. They are supplemented with a style of documentational block comments that aim to describe their usage as accurate and clear as possible. A block comment may contain the following tags:

  * @brief
  * @param
  * @return
  * @note
  * @bug

## Build

The _tools_ folder contains an ATMEL Studio project that may be used to build the test program located in _tests_. The project is setup to link to the files in _inc_ and _src_, thus avoiding unnecessary copies. Its build-in compiler includes _inc_ as well.

A makefile is provided, but is not setup to generate a programmable image. By using _make_, one may easily check whether the program compiles correctly and without warnings.

## Devices

Device | Status
--- | ---
AT90CAN128 | Supported
ATMEGA32M1 | Untested