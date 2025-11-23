# OneNee_402
PsNee + OneChip port for the ATtiny402 on PAL PM41 boards.

Port of PsNee v7/v8 with OneChip BIOS patching for the ATtiny402.
Full stealth and BIOS patching on PAL PM41-v1 and PM41-v2 boards! There is virtually no noise or degradation of the laser RF signal level because the code injects the SCEX string only when needed.
This chip is ONLY compatible with PM-41v1 and PM-41v2 PAL motherboards.

I'm just a hobbyist programmer, so please forgive any coding mistakes, syntax errors, or formal issues.
The code is heavily based on PsNee v7 by kalymos, but I chose to follow the Arduino philosophy.
I prioritized easy portability of the code while still using direct bit manipulation for GPIO operations to avoid as much of the Arduino platform’s “heaviness” as possible.

For the experts:
Yes! I am using the PA0/UPDI pin. You can use PA0 as an input without needing high-voltage (HV) programming as long as you keep the UPDI fuse enabled and do not disable the UPDI function.

## Features
- Full Stealth.
- PAL Bios patching feature for USA and JAP games.
- Virtually no noise and degradation on the laser RF signal level unlike the old onechip mod.
- No atmega328p+16mhz cristal or other arduino uno like "big" board, simple a 8-pin ATtiny 402 chip to solder and easly fit in your console!

## Supported Playstations
- PSone SCPH-102 ONLY!.
- For other models, you can use my [PsNee_402](https://github.com/carmax91/PsNee_402) port.

## Prerequisites
- Arduino IDE 2.3+ or newer.
- **megaTinyCore** library: https://github.com/SpenceKonde/megaTinyCore
- An UPDI programmer (you can build  one with a 1$ CH340 and a diode).
  Read the [SerialUPDI](https://github.com/SpenceKonde/AVR-Guidance/blob/master/UPDI/jtag2updi.md) documentation to learn how to build one..

## HowTo
- Install the Arduino IDE.
- Install the SpenceKonde [megaTinyCore](https://github.com/SpenceKonde/megaTinyCore) library.
- Download the [OneNee.ino](OneNee_402/OneNee_402.ino) file.
- Connect the UPDI programmer.
- Open the OneNee.ino file in the Arduino IDE and select the board:
  (Tools -> Board -> megaTinyCore -> ATtiny412/402/212/202)
- **FIRST and before** uploading the sketch, set the correct settings as shown in the image above::
![Setting](https://github.com/carmax91/OneNee_402/blob/main/Imgs/OneNee%20settings.jpg?raw=true)
- **Then** select “Burn Bootloader”.
- After that, choose Sketch -> Upload Using Programmer.
- For installation, follow the wiring diagrams in the [Install](Install) folder based on your console motherboard.

## Why this modc. is better and different compared to the older oldcrow\mayumi\multimode\onchip\stealth2.x? (extracted part from PsNeev7 readme)

The original code doesn't have a mechanism to turn the injections off. It bases everything on a timer. After power on, it will start sending injections for some time, then turns off. 
It also doesn't know when it's required to turn on again (except for after a reset), so it gets detected by anti-mod games.
The mechanism to know when to inject and when to turn it off.

This is the 2 wires for SUBQ / SQCK. The PSX transmits the current subchannel Q data on this bus. It tells the console where on the disc the read head is. We know that the protection symbols only exist on the earliest sectors, and that anti-mod games exploit this by looking for the symbols elsewhere on the disk. If they get those symbols, a modchip must be generating them!

So with that information, my code knows when the PSX wants to see the unlock symbols, and when it's "fake" / anti-mod. The chip is continously looking at that subcode bus, so you don't need the reset wire or any other timing hints that other modchips use. That makes it compatible and fully functional with all revisions of the PSX, not just the later ones. Also with this method, the chip knows more about the current CD. This allows it to not send unlock symbols for a music CD, which means the BIOS starts right into the CD player, instead of after a long delay with other modchips.

This has some drawbacks, though:

    It's more logic / code. More things to go wrong. The testing done so far suggests it's working fine though.
    It's not a good example anymore to demonstrate PSX security, and how modchips work in general.


## Thanks
ramapcsx2, kalymos, SpenceKonde, oldcrow, mayumi, arduino community and lots of people that can't remeber now.



