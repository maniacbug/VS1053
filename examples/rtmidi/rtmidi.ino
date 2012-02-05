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

// 'buttons' >= 0x80 are analog inputs, e.g. 0x85 will result in checking
// analogRead(5).
static uint8_t buttons[] = { A0, A1, A2, A3, A4, 0x85 };
static uint8_t notes[] = { 
  41, // Low Floor Tom
  47, // Low mid tom
  48, // High mid tom
  38, // Acoustic Snare 
  31, // Sticks
  49, // Crash Cymbal 1  
};
static bool last_state[6];
static uint32_t last_triggered_at[6];

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

  // Setup pins
  int i = sizeof(buttons);
  while(i--)
  {
    if ( buttons[i] < 0x80 )
    {
      pinMode(buttons[i],INPUT);
      digitalWrite(buttons[i],HIGH);
    }
    midi.selectDrums(i);
  }
  
  Serial.println("+READY");
}

void loop(void)
{
  // Each button is on its own channel.  Update each one
  // separately.
  int channel = sizeof(buttons);
  while (channel--)
  {
    // Check status of corresponding button
    bool state;
    uint8_t button = buttons[channel];
    if ( button < 0x80 )
      state = ! digitalRead(button);
    else
    {
      // For piezos, ignore state changes in the first <interval> ms.
      const uint32_t ignore_interval = 50;
      if ( last_state[channel] && ( millis() - last_triggered_at[channel] < ignore_interval ) )
	state = true;
      else
	state = analogRead(button & 0x7f );
    }

    if ( state != last_state[channel] )
    {
      last_state[channel] = state;
      if ( state )
      {
	last_triggered_at[channel] = millis();
	midi.noteOn(0,notes[channel],0x7f);
      }
      else
      {
	last_triggered_at[channel] = 0; 
	midi.noteOff(0,notes[channel]);
      }
    }
  }
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

    if ( c >= '1' && c <= ('0' + (int)sizeof(notes) ) )
    {
      uint8_t note = notes[c - '1'];
      printf_P(PSTR(": note %u\r\n"),note);
      midi.noteOn(0,note,0x7f);
      delay(250);
      midi.noteOff(0,note);
    }
  }
}

// vim:cin:ai:sts=2 sw=2 ft=cpp
