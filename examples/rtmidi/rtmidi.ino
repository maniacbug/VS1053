/*
 Copyright (C) 2011 J. Coliz <maniacbug@ymail.com>

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
#include "rtmidistart_plg.h"


#undef PSTR
#define PSTR(s) (__extension__({static const char __c[] __attribute__ (( section (".progmem") )) = (s); &__c[0];}))

/**
* MP3 player instance
*/
VS1053 player(/* cs_pin */ 10,/* dcs_pin */ 7,/* dreq_pin */ 4,/* reset_pin */ 5);

class RtMidi
{
private:
  VS1053& player;
  uint8_t buf[6];
protected:
  void write(uint8_t a, uint8_t b, uint8_t c)
  {
    buf[1] = a;
    buf[3] = b;
    buf[5] = c;
    player.playChunk(buf,sizeof(buf));
  }
  void write(uint8_t a, uint8_t b)
  {
    buf[1] = a;
    buf[3] = b;
    player.playChunk(buf,4);
  }
public:
  RtMidi(VS1053& _player): player(_player) 
  {
    memset(buf,0,6);
  }
  void noteOn(uint8_t channel, uint8_t note, uint8_t volume)
  {
    write(channel | 0x90, note, volume);
  }
  void noteOff(uint8_t channel, uint8_t note)
  {
    write(channel | 0x80, note, 0x45);
  }
  void selectDrums(void)
  {
    write(0xB0, 0, 0x78);
    write(0xC0,30);
  }
};

RtMidi midi(player);
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
  
  Serial.println("+READY");
  while ( ! Serial.available() );
  Serial.read();

  SPI.begin();
  player.begin();
  printf_P(PSTR("Loading user code..."));
  player.loadUserCode(plugin,PLUGIN_SIZE);
  //printf_P(PSTR("+OK\r\n"));
  player.setVolume(0x0);
  midi.selectDrums();
}


const uint8_t lowest_note = 27;
const uint8_t highest_note = 87;
const uint8_t note_on = 0x90;
const uint8_t note_off = 0x80;
uint8_t current_note = lowest_note;

void loop(void)
{
  printf_P(PSTR("ON %x\r\n"),current_note);
  midi.noteOn(0,current_note,0x7f);
  delay(500);
  printf_P(PSTR("OFF %x\r\n"),current_note);
  midi.noteOff(0,current_note);

  if ( ++current_note > highest_note )
    current_note = lowest_note;
}

// vim:cin:ai:sts=2 sw=2 ft=cpp
