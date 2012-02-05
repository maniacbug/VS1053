/*
 Copyright (C) 2012 J. Coliz <maniacbug@ymail.com>

 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 version 2 as published by the Free Software Foundation.
 */

// STL headers
// C headers
#include <avr/pgmspace.h>
// Framework headers
// Library headers
#include <VS1053.h>
#include <SPI.h>
// Project headers
#include "printf.h"

#undef PSTR
#define PSTR(s) (__extension__({static const char __c[] __attribute__ (( section (".progmem") )) = (s); &__c[0];}))

/**
* MP3 player instance
*/
VS1053 player(/* cs_pin */ 10,/* dcs_pin */ 7,/* dreq_pin */ 4,/* reset_pin */ 5);

/**
 * Real-time MIDI player instance
 */
VS1053::RtMidi midi(player);

/**
 * Instrument details, one on each button
 */

struct instrument_t
{
  uint8_t button; /**< Digital pin# or 0x80 + analog pin# */
  uint8_t note; /**< MIDI note # */
  bool last_state; /**< Last time we were updated, were we 'on'? */
  uint32_t last_triggered_at; /**< When we were last triggered? */

  void begin(void);
  void update(void);
};

instrument_t instruments[] = {
  { A0, 41, 0, 0 },
  { A1, 47, 0, 0 },
  { A2, 48, 0, 0 },
  { A3, 38, 0, 0 },
  { A4, 31, 0, 0 },
  { 0x85, 49, 0, 0 },
};
const int num_instruments = sizeof(instruments)/sizeof(instruments[0]);

void instrument_t::begin(void)
{
  if ( button < 0x80 )
  {
    pinMode(button,INPUT);
    digitalWrite(button,HIGH);
  }
}

void instrument_t::update(void)
{
  // Check status of corresponding button
  bool state;
  if ( button < 0x80 )
    state = ! digitalRead(button);
  else
  {
    // For piezos, ignore state changes in the first <interval> ms.
    const uint32_t ignore_interval = 50;
    if ( last_state && ( millis() - last_triggered_at < ignore_interval ) )
      state = true;
    else
      state = analogRead(button & 0x7f );
  }

  if ( state != last_state )
  {
    last_state = state;
    if ( state )
    {
      last_triggered_at = millis();
      midi.noteOn(0,note,0x7f);
    }
    else
    {
      last_triggered_at = 0; 
      midi.noteOff(0,note);
    }
  }
}

//
// Application
//

extern uint16_t* __brkval;

void setup(void)
{
  // Preamble
  Serial.begin(57600);
  printf_begin();
  printf_P(PSTR(__FILE__ "\r\n"));
  printf_P(PSTR("FREE=%u\r\n"),SP-(uint16_t)__brkval);
  
  SPI.begin();
  player.begin();
  midi.begin();
  player.setVolume(0x0);
  midi.selectDrums(0);

  // Setup pins
  int i = num_instruments;
  while(i--)
    instruments[i].begin();
  
  Serial.println("+READY");
}

void loop(void)
{
  // Update each instrument independently
  int i = num_instruments; 
  while (i--)
    instruments[i].update();
}

void loop_scale(void)
{
  const uint8_t lowest_note = 27;
  const uint8_t highest_note = 87;
  static uint8_t current_note = lowest_note;

  printf_P(PSTR("ON %x\r\n"),current_note);
  midi.noteOn(0,current_note,0x7f);
  delay(500);
  printf_P(PSTR("OFF %x\r\n"),current_note);
  midi.noteOff(0,current_note);

  if ( ++current_note > highest_note )
    current_note = lowest_note;
}

void loop_piezo(void)
{
  // test piezo
  const uint32_t interval = 10;
  uint32_t now = millis();
  static uint32_t last_reading = 0;
  if ( last_reading - now > interval )
  {
    last_reading = now;
    unsigned reading = analogRead(5);
    if ( reading )
      printf_P(PSTR("%02x "),reading>>2);
  }
}

void loop_serial(void)
{
  if ( Serial.available() )
  {
    char c = Serial.read();
    Serial.print(c);

    if ( c >= '1' && c <= ('0' + (int)num_instruments ) )
    {
      uint8_t note = instruments[c - '1'].note;
      printf_P(PSTR(": note %u\r\n"),note);
      midi.noteOn(0,note,0x7f);
      delay(250);
      midi.noteOff(0,note);
    }
  }
}

// vim:cin:ai:sts=2 sw=2 ft=cpp
