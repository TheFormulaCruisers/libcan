# libcan

This library simplifies CAN bus communication on a range of Microchip's 8-bit AVR microcontrollers. It supports sending and receiving messages of variable length using either the 2.0A or the 2.0B CAN bus communicaton protocol. The bitrate is currently fixed at its highest value, which on a 16MHz clocked MCU corresponds to 1Mbps.

## Operation

...

## Usage

Functions are defined in the header files located in _inc_. Each function is supplemented with documentational block comments that describe their usage. A block comment may contain the following tags:

  * @brief
  * @param
  * @return
  * @note
  * @bug

## Macros

The following macros may be specified when compiling the source:

  * CAN_REV_2A or CAN_REV_2B
  * CAN_RX_MSGBUF_SIZE
  * CAN_TX_MSGBUF_SIZE

## Build

The _tools_ folder contains an ATMEL Studio project that may be used to build the test program located in _tests_. The project is setup to refer to the original files in _inc_ and _src_, thus avoiding unnecessary copies. Its build-in compiler includes _inc_ as well (_Properties > Toolchain > Directories_). Macros are defined at _Properties > Toolchain > Symbols_.

A makefile is provided, but is not setup to generate a programmable image. By using _make_, one may easily check whether the program compiles correctly and without warnings using a terminal or command-prompt.

## Devices

Device | Status
--- | ---
AT90CAN128 | Supported
ATMEGA32M1 | Untested