# libcan

This library simplifies CAN bus communication on a range of Microchip's 8-bit AVR microcontrollers. It supports sending and receiving messages of variable length using either the 2.0A or the 2.0B CAN bus communicaton protocol. The bitrate is currently fixed at its highest value, which on a 16MHz clocked MCU corresponds to 1Mbps.

## Operation

Build into some AVR microcontrollers is a module for handling communication on a Controller Area Network parallel to program execution. It supports generating interrupts on reception, transmission and various errors and warnings. Once enabled, a message is stored in one of the Message Objects (MObs) that are built into the hardware. A software routine may then collect a message from a MOb and re-enable it for reception. A message may be catched when its identifier (partially) matches with one that has been preconfigured in the MOb.

Libcan implements an additional buffer between the Message Objects and its interface, allowing for received messages to be updated and to build up a transmission qeue. The library uses transmission and reception interrupts to copy data from the transmission buffer to a transmission MOb and from a receiving MOb to the reception buffer. The interface contains functions that copy data from the calling function to the transmission buffer and from the reception buffer to the calling function and checking whether new messages have been received. 

## Usage

Functions are defined in the header files located in _inc_. Each function is supplemented with documentational block comments that describe their usage. A block comment may contain the following tags:

  * @brief
  * @param
  * @return
  * @note
  * @bug

## Macros

The following macros may be specified when compiling the source:

  * CAN\_REV\_2A or CAN\_REV\_2B
  * CAN\_RX\_MSGBUF\_SIZE
  * CAN\_TX\_MSGBUF\_SIZE

## Build

The _tools_ folder contains an ATMEL Studio project that may be used to build the test program located in _tests_. The project is setup to link to the original files in _inc_ and _src_, thus avoiding unnecessary copies. Its build-in compiler includes _inc_ as well (_Properties > Toolchain > Directories_). Macros are defined at _Properties > Toolchain > Symbols_.

A makefile is provided, but is not setup to generate a programmable image. By using _make_, one may easily check whether the program compiles correctly and without warnings using a terminal or command-prompt.

## Devices

Device | Status
--- | ---
AT90CAN128 | Supported
ATMEGA32M1 | Untested
