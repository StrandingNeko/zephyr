.. _tl3218x:

Telink TL3218X
#####################

Overview
********

The TL3218X Generic Starter Kit is a hardware platform which
can be used to verify the Telink TLx series chipset and develop applications
for several 2.4 GHz air interface standards including Bluetooth LE, Zigbee, RF4CE,
Thread, Matter, and 2.4GHz proprietary standard.

.. figure:: img/tl3218x.jpg
     :align: center
     :alt: TL3218X

.. More information about the board can be found at the `Telink B92 Generic Starter Kit Hardware Guide`_ website.

Hardware
********

The TL3218X SoC integrates a powerful 32-bit RISC-V MCU, DSP, 2.4 GHz ISM Radio, 128 KB SRAM
including 96 KB retention feature SRAM, external Flash memory, 12-bit AUX ADC, PWM, flexible
IO interfaces, and other peripheral blocks required for advanced IoTapplications.

.. figure:: img/tl3218_block_diagram.png
     :align: center
     :alt: TL3218X_SOC

The TL3218X default board configuration provides the following hardware components:

- RF conducted antenna
- 2 MB External SPI Flash memory with reset button. (Possible to mount 1/2/4 MB)
- Chip reset button
- Mini USB interface
- 4-wire JTAG
- 4 LEDs, Key matrix up to 4 keys
- Stereo line-out

Supported Features
==================

The Zephyr TL3218X board configuration supports the following hardware features:

+----------------+------------+------------------------------+
| Interface      | Controller | Driver/Component             |
+================+============+==============================+
| PLIC           | on-chip    | interrupt_controller         |
+----------------+------------+------------------------------+
| RISC-V Machine | on-chip    | timer                        |
| Timer (32 KHz) |            |                              |
+----------------+------------+------------------------------+
| PINCTRL        | on-chip    | pinctrl                      |
+----------------+------------+------------------------------+
| GPIO           | on-chip    | gpio                         |
+----------------+------------+------------------------------+
| UART           | on-chip    | serial                       |
+----------------+------------+------------------------------+
| PWM            | on-chip    | pwm                          |
+----------------+------------+------------------------------+
| TRNG           | on-chip    | entropy                      |
+----------------+------------+------------------------------+
| FLASH (MSPI)   | on-chip    | flash                        |
+----------------+------------+------------------------------+
| RADIO          | on-chip    | Bluetooth,                   |
|                |            | ieee802154, OpenThread       |
+----------------+------------+------------------------------+
| SPI (Master)   | on-chip    | spi                          |
+----------------+------------+------------------------------+
| I2C (Master)   | on-chip    | i2c                          |
+----------------+------------+------------------------------+
| ADC            | on-chip    | adc                          |
+----------------+------------+------------------------------+
| USB (device)   | on-chip    | usb_dc                       |
+----------------+------------+------------------------------+
| AES            | on-chip    | mbedtls                      |
+----------------+------------+------------------------------+
| PKE            | on-chip    | mbedtls                      |
+----------------+------------+------------------------------+

.. Board supports power-down modes: suspend and deep-sleep. For deep-sleep mode only 96KB of retention memory is available.

Board supports HW cryptography acceleration (AES and ECC till 256 bits). MbedTLS interface is used as cryptography front-end.

.. note::
   To support "button" example project PD6-KEY3 (J5-13, J5-14) jumper needs to be removed and KEY3 (J5-13) should be connected to GND (J3-30) externally.

   For the rest example projects use the default jumpers configuration.

Limitations
-----------

- Maximum 3 GPIO ports could be configured to generate external interrupts simultaneously. All ports should use different IRQ numbers.
- DMA mode is not supported by I2C, SPI and Serial Port.
- SPI Slave mode is not implemented.
- I2C Slave mode is not implemented.
- Bluetooth is not compatible with deep-sleep mode. Only suspend is allowed when Bluetooth is active.
- USB working only in active mode (No power down supported).
- During deep-sleep all GPIO's are in Hi-Z mode.
- Shell is not compatible with sleep modes.

Default configuration and IOs
=============================

System Clock
------------

The TL3218X board is configured to use the 24 MHz external crystal oscillator
with the on-chip PLL/DIV generating the 96 MHz system clock.
The following values also could be assigned to the system clock in the board DTS file
(``boards/riscv/tl321x/tl3218x-common.dtsi``):

- 24000000
- 48000000
- 96000000

.. code-block::

   &cpu0 {
       clock-frequency = <96000000>;
   };

PINs Configuration
------------------

The TL3218X SoC has five GPIO controllers (PORT_A to PORT_F), and the next are
currently enabled:

- LED0 (white): PD0, LED1 (green): PB0, LED2 (red): PB1, LED3 (blue): PB2
- Key Matrix SW3: PB3_PB6, SW4: PB3_PB7, SW5: PB5_PB6, SW6: PB5_PB7

Peripheral's pins on the SoC are mapped to the following GPIO pins in the
``boards/riscv/tl321x/tl3218x-common.dtsi`` file:

- UART0 TX: PE0, RX: PE1
- PWM Channel 0: PB2
- GSPI CLK: PE5, MISO: PE6, MOSI: PE7
- I2C SCL: PC3, SDA: PC2

Serial Port
-----------

The TL3218X SoC has 1 UART. The Zephyr console output is assigned to UART0.
The default settings are 115200 8N1.

Programming and debugging
*************************

Building
========

.. important::

   These instructions assume you've set up a development environment as
   described in the `Zephyr Getting Started Guide`_.

To build applications using the default RISC-V toolchain from Zephyr SDK, just run the west build command.
Here is an example for the "hello_world" application.

.. code-block:: console

   # From the root of the zephyr repository
   west build -b tl3218x samples/hello_world

Open a serial terminal with the following settings:

- Speed: 115200
- Data: 8 bits
- Parity: None
- Stop bits: 1

Flash the board, reset and observe the following messages on the selected
serial port:

.. code-block:: console

   *** Booting Zephyr OS build zephyr-v3.3.0-xxxx-xxxxxxxxxxxxx  ***
   Hello World! tl3218x


Flashing
========

To flash the TL3218X board see the sources below:

- `Burning and Debugging Tools for all Series`_

.. It is also possible to use the west flash command. Download BDT tool for Linux `Burning and Debugging Tools for Linux`_ or
.. `Burning and Debugging Tools for Windows`_ and extract archive into some directory you wish TELINK_BDT_BASE_DIR

.. - Now you should be able to run the west flash command with the BDT path specified (TELINK_BDT_BASE_DIR).

.. .. code-block:: console

..    west flash --bdt-path=$TELINK_BDT_BASE_DIR --erase

.. - You can also run the west flash command without BDT path specification if TELINK_BDT_BASE_DIR is in your environment (.bashrc).

.. .. code-block:: console

..    export TELINK_BDT_BASE_DIR="/opt/telink_bdt/"


References
**********

.. target-notes::

.. _Burning and Debugging Tools for all Series: https://wiki.telink-semi.cn/wiki/IDE-and-Tools/Burning-and-Debugging-Tools-for-all-Series/
.. _Burning and Debugging Tools for Linux: https://wiki.telink-semi.cn/tools_and_sdk/Tools/BDT/Telink_libusb_BDT-Linux-X64-V1.6.0.zip
.. _Burning and Debugging Tools for Windows: https://wiki.telink-semi.cn/tools_and_sdk/Tools/BDT/BDT.zip
.. _Zephyr Getting Started Guide: https://docs.zephyrproject.org/latest/getting_started/index.html
