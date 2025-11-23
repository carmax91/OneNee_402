// One Nee porting of onechip bios patching to PsNee / psxdev.net version
// Porting to Arduino ATtiny 402 by Carmax91 based on rev2
//
// There are some pictures in the development thread ( http://www.psxdev.net/forum/viewtopic.php?f=47&t=1262&start=120 )
// Beware to use the PSX 3.5V / 3.3V power, *NOT* 5V! The installation pictures include an example.
//
// Only for arduino ATtiny 402
//
// Install SpenceKonde megaTinyCore library for ARDUINO. Install the library via this link: https://github.com/SpenceKonde/megaTinyCore
//
// PAL PM-41 consoles BIOS supported only.
// Also, the Arduino bootloader settings must be flashed FIRST and only after the sketch can be uploaded via an UPDI programmer.
//
// The settings for the Arduino IDE enviroment are:
// BOARD: ATtiny412/402/212/202 (without bootloader)
// CHIP: ATtiny 402
// FREQUENCY: 20 Mhz (internal)
// BOD MODE: Disabled
// BOD VOLTAGE: 1.8v
// SAVE EEPROM: Indifferent setting…
// MILLIS()/MICROS(): Enabled
// PRINTF(): Default
// PWM PINS: Default (PA1-3, 7)
// STARTUP TIME: 8Ms
// WDT TIMEOUT: Disabled
// WDT WINDOW: No Delay
// WIRE (WIRE.H/I2C): Master or Slave (Saves RAM)
//
// PINOUT:
/*
                   ___ ___
             VCC -|   U   |- GND
  Bios_A18 (PA6) -|       |- Bios_D2 (PA3)
      DATA (PA7) -|       |- SUBQ / UPDI PORT (PA0)
 GATE/WFCK (PA1) -|       |- SQCK (PA2)
                   ------- 

*/

//_______________________PREDIRECTIVES SETTINGS_________________________________________________________________________
#define DF_CPU 20000000L  //Speed 20Mhz. (Don't forget to set FREQUENCY -> 20Mhz internal!)
#define HYSTERESIS_MAX 15  //Value of hysteresys, bad drive need higher value max at 19
//______________________________________________________________________________________________________________________

// Board pins
#define sqck PIN_PA2
#define subq PIN_PA0
#define data PIN_PA7
#define gate_wfck PIN_PA1
#define bios_D2 PIN_PA3
#define bios_A18 PIN_PA6

#define NOP __asm__ __volatile__("nop\n\t")

// Setup() detects which (of 2) injection methods this PSX board requires, then stores it in pu22mode.
boolean pu22mode;

// Timings
const int delay_between_bits = 4000;      // 250 bits/s (microseconds) (ATtiny 8Mhz works from 3950 to 4100)
const int delay_between_injections = 90;  // 72 in oldcrow. PU-22+ work best with 80 to 100 (milliseconds)
const int delay_BFix = 2600; //Optimal value for NTSC Bios fix delay on ATtiny_402 at 20Mhz

// borrowed from AttyNee. Bitmagic to get to the SCEX strings stored in flash (because Harvard architecture)
bool readBit(int index, const unsigned char *ByteSet) {
  int byte_index = index >> 3;
  byte bits = pgm_read_byte(&(ByteSet[byte_index]));
  int bit_index = index & 0x7;  // same as (index - byte_index<<3) or (index%8)
  byte mask = 1 << bit_index;
  return (0 != (bits & mask));
}

void inject_SCEX(char region) {
  //SCEE: 1 00110101 00, 1 00111101 00, 1 01011101 00, 1 01011101 00
  //SCEA: 1 00110101 00, 1 00111101 00, 1 01011101 00, 1 01111101 00
  //SCEI: 1 00110101 00, 1 00111101 00, 1 01011101 00, 1 01101101 00
  //const boolean SCEEData[44] = {1,0,0,1,1,0,1,0,1,0,0,1,0,0,1,1,1,1,0,1,0,0,1,0,1,0,1,1,1,0,1,0,0,1,0,1,0,1,1,1,0,1,0,0};
  //const boolean SCEAData[44] = {1,0,0,1,1,0,1,0,1,0,0,1,0,0,1,1,1,1,0,1,0,0,1,0,1,0,1,1,1,0,1,0,0,1,0,1,0,1,1,1,0,1,0,0};
  //const boolean SCEIData[44] = {1,0,0,1,1,0,1,0,1,0,0,1,0,0,1,1,1,1,0,1,0,0,1,0,1,0,1,1,1,0,1,0,0,1,0,1,0,1,1,1,0,1,0,0};
  static const PROGMEM unsigned char SCEEData[] = { 0b01011001, 0b11001001, 0b01001011, 0b01011101, 0b11101010, 0b00000010 };
  static const PROGMEM unsigned char SCEAData[] = { 0b01011001, 0b11001001, 0b01001011, 0b01011101, 0b11111010, 0b00000010 };
  static const PROGMEM unsigned char SCEIData[] = { 0b01011001, 0b11001001, 0b01001011, 0b01011101, 0b11011010, 0b00000010 };

  // pinMode(data, OUTPUT) is used more than it has to be but that's fine.
  for (byte bit_counter = 0; bit_counter < 44; bit_counter++) {
    if (readBit(bit_counter, region == 'e' ? SCEEData : region == 'a' ? SCEAData : SCEIData) == 0) {
      pinModeFast(data, OUTPUT);
      digitalWriteFast(data, 0);  // data low
      delayMicroseconds(delay_between_bits);
    } 
    else {
          if (pu22mode) {
        pinModeFast(data, OUTPUT);
        unsigned long now = micros();
        do {
          bool wfck_sample = digitalReadFast(gate_wfck);  //Put readed bool value (1 or 0) into wfck_sample var
        //ouput wfck_sample var on data pin
        if (wfck_sample == 1){
              digitalWriteFast(data, 1);
            }
            else{
              digitalWriteFast(data, 0);
            }
        } 
        while ((micros() - now) < delay_between_bits);
      } 
      else {  // PU-18 or lower mode
        pinModeFast(data, INPUT);
        delayMicroseconds(delay_between_bits);
      }
    }
  }

  pinModeFast(data, OUTPUT);
  digitalWriteFast(data, 0);  // pull data low
  delay(delay_between_injections);
}

void NTSC_fix() {
  pinModeFast(bios_A18, INPUT);
  pinModeFast(bios_D2, INPUT);
  
  delay(delay_BFix); //Optimized delay for ATtiny_402 MCU

  noInterrupts(); // start critical section
  
  while (digitalReadFast(bios_A18) == 0) 
  {
    ;  //wait for priming A18 pulse
  }
  delayMicroseconds(12); //min 13us max 17us for 16Mhz ATmega (maximize this when tuning!)
  digitalWriteFast(bios_D2, 0); // store a low
  pinModeFast(bios_D2, OUTPUT); // D2 = output. Drags line low now
  delayMicroseconds(5); //min 2us for 16Mhz ATmega, 8Mhz requires 3us (minimize this when tuning, after maximizing first us delay!)
  pinModeFast(bios_D2, INPUT); // D2 = input / high-z
  interrupts(); // end critical section

  // not necessary but I want to make sure these pins are now high-z again
  pinModeFast(bios_A18, INPUT);
  pinModeFast(bios_A18, INPUT);
}

//--------------------------------------------------
//     Setup
//--------------------------------------------------

void setup() {
  pinModeFast(data, INPUT);
  pinModeFast(gate_wfck, INPUT);
  pinModeFast(subq, INPUT);  // PSX subchannel bits
  pinModeFast(sqck, INPUT);  // PSX subchannel clock

  NTSC_fix(); //Start NTSC BiosFix before sqck and wfck stabilization.

  // wait for console power on and stable signals
  while (!digitalReadFast(sqck));
  while (!digitalReadFast(gate_wfck));

  //Forcing to PU22 mode because we are using PM-41 board! Detection instructions are not needed!
  pu22mode = 1;

}

void loop() {
  static byte scbuf[12] = { 0 };  // We will be capturing PSX "SUBQ" packets, there are 12 bytes per valid read.
  static unsigned int timeout_clock_counter = 0;
  static byte bitbuf = 0;  // SUBQ bit storage
  static byte bitpos = 0;
  byte scpos = 0;  // scbuf position

  // start with a small delay, which can be necessary in cases where the MCU loops too quickly
  // and picks up the laster SUBQ trailing end
  delay(1);

  noInterrupts();  // start critical section
start:
  // Capture 8 bits for 12 runs > complete SUBQ transmission
  bitpos = 0;
  for (; bitpos < 8; bitpos++) {
    while (digitalReadFast(sqck) != 0) {  
      // wait for clock to go low..
      // a timeout resets the 12 byte stream in case the PSX sends malformatted clock pulses, as happens on bootup
      timeout_clock_counter++;
      if (timeout_clock_counter > 1000) {
        scpos = 0;  // reset SUBQ packet stream
        timeout_clock_counter = 0;
        bitbuf = 0;
        goto start;
      }
    }

    // wait for clock to go high..
    while (digitalReadFast(sqck) == 0); 
                                     /*
    sample = digitalRead (subq);
    bitbuf |= sample << bitpos;

    timeout_clock_counter = 0; // no problem with this bit
  */
    if (digitalReadFast(subq) == 1)  // while clock pin high
    {
      bitbuf |= 1 << bitpos;  // Set the bit at position bitpos in the bitbuf to 1. Using OR combined with a bit shift
    }

    timeout_clock_counter = 0;  // no problem with this bit
  }

  // one byte done
  scbuf[scpos] = bitbuf;
  scpos++;
  bitbuf = 0;

  // repeat for all 12 bytes
  if (scpos < 12) {
    goto start;
  }
  interrupts();  // end critical section

  // check if read head is in wobble area
  // We only want to unlock game discs (0x41) and only if the read head is in the outer TOC area.
  // We want to see a TOC sector repeatedly before injecting (helps with timing and marginal lasers).
  // All this logic is because we don't know if the HC-05 is actually processing a getSCEX() command.
  // Hysteresis is used because older drives exhibit more variation in read head positioning.
  // While the laser lens moves to correct for the error, they can pick up a few TOC sectors.
  static byte hysteresis = 0;
  boolean isDataSector = (((scbuf[0] & 0x40) == 0x40) && (((scbuf[0] & 0x10) == 0) && ((scbuf[0] & 0x80) == 0)));

  if (
    (isDataSector && scbuf[1] == 0x00 && scbuf[6] == 0x00) &&       // [0] = 41 means psx game disk. the other 2 checks are garbage protection
    (scbuf[2] == 0xA0 || scbuf[2] == 0xA1 || scbuf[2] == 0xA2 ||    // if [2] = A0, A1, A2 ..
     (scbuf[2] == 0x01 && (scbuf[3] >= 0x98 || scbuf[3] <= 0x02)))  // .. or = 01 but then [3] is either > 98 or < 02
  ) 
  {
    hysteresis++;
  } 
  else if (hysteresis > 0 && ((scbuf[0] == 0x01 || isDataSector) && (scbuf[1] == 0x00 /*|| scbuf[1] == 0x01*/) && scbuf[6] == 0x00)) {  // This CD has the wobble into CD-DA space. (started at 0x41, then went into 0x01)
    hysteresis++;
  } 
  else if (hysteresis > 0) {
    hysteresis--;  // None of the above. Initial detection was noise. Decrease the counter.
  }

  // hysteresis value "optimized" using very worn but working drive on ATmega328 @ 16Mhz
  // should be fine on other MCUs and speeds, as the PSX dictates SUBQ rate
  if (hysteresis >= HYSTERESIS_MAX) {
    // If the read head is still here after injection, resending should be quick.
    // Hysteresis naturally goes to 0 otherwise (the read head moved).
    hysteresis = 11;

    //Injecting!

    pinModeFast(data, OUTPUT);
    digitalWriteFast(data, 0);  // Pull data low
    if (!pu22mode) {
      pinModeFast(gate_wfck, OUTPUT);
      digitalWriteFast(gate_wfck, 0);
    }

    // HC-05 waits for a bit of silence (pin low) before it begins decoding.
    delay(delay_between_injections);
    // Inject symbols now. 2 x 3 runs seems optimal to cover all boards
    for (byte loop_counter = 0; loop_counter < 2; loop_counter++) {
      inject_SCEX('e');  // e = SCEE, a = SCEA, i = SCEI
      inject_SCEX('e');  // optimized boot time by sending
      inject_SCEX('e');  // only PAL console region letter (all 3 times per loop)
    }

    if (!pu22mode) {
      pinModeFast(gate_wfck, INPUT);  // high-z the line, we're done
    }
    pinModeFast(data, INPUT);        // high-z the line, we're done
    //Injection done!
  }
  // keep catching SUBQ packets forever
}
