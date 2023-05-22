/* Arduino Nano AY-3-8910 Audio Player,
 * Version 1.0, (C)2023 Andreas J. Reichel
 * MIT License */
#define BDIR      PD3
#define BC1       PD2

#define DATA_LOW  PORTB   /* bits 0, 2 - 5 */
#define DATA_HI6  PC0
#define DATA_HI7  PC1     /* bits 6 - 7 */

#define RESET     PD5
#define CLOCK     PB1

#define MAX_FRAME_BYTES 56
#define MAX_FRAME_OFFSET_US 15000 /* latest instruction time within frame in micro seconds */

/******** AY-3-8910 Functions *******/
void reset() { PORTD &= ~_BV(RESET); delay(1); PORTD |= _BV(RESET); }
void inact() { PORTD &= ~(_BV(BDIR) | _BV(BC1)); }
void writedata() { PORTD |= _BV(BDIR); }
void latchaddr() { PORTD |= _BV(BDIR) | _BV(BC1); }

void write_byte(uint8_t val) {
  /* writes a byte to the stupid bus mapping */
  /* clear all bits that belong to the byte */
  /* 0011 1101  = 0x3D, bit 1 is in other port */
  DATA_LOW &= ~0x3D;
  DATA_LOW |= val & 0x3D;
  
  /* clear PORTD (only bit 6 we write to) */
  /* 0x0100 0000 = 0xBF */
  PORTD &= ~0x40;
  PORTD |= (val & 2) << 5;

  /* and now the high 2 bits, written to PORTC bits 0 and 1 */
  PORTC &= ~0x03;
  PORTC |= (val >> 6) & 0x03;
}

void write_psg(uint8_t reg, uint8_t data) {
  inact();
  write_byte(reg);
  latchaddr();
  inact();
  write_byte(data);
  writedata();
  inact();
}

void setup() {
  DDRC = 0xFF;
  DDRB = 0xFF;
  DDRD = 0xFF;

  /* Generate 1.6 MHz for CLOCK of AY-3-8910 on OC1A pin */
  TCCR1A = _BV(COM1A0);
  TCCR1B = _BV(WGM12) | _BV(CS10);
  OCR1A = 4;
  
  inact();
  reset();

  /* We need a high Baud Rate to transfer 48 bytes on the beginning
     of every frame maximum */
  Serial.begin(500000);

  noInterrupts();
  /* Configure Timer2 Compare Match Interrupt to count up frame_timer up to 157
     every 20 Milliseconds, this gives the audio frame timing of 50 Hz */
  TCCR2A = 0;
  TCNT2  = 0;
  OCR2A = 0;
  TCCR2B = (0 << CS22) | (1 << CS21) | (0 << CS20);
  TIMSK2 |= (1 << OCIE2A);
  interrupts();
}

volatile uint16_t frame_timer = 0;

ISR(TIMER2_COMPA_vect)                    
{
  frame_timer++;
}

uint8_t get_buffer_byte()
{
  while (Serial.available() < 1) {
    write_psg(7, 0xFF); /* turn sound off */  
  };
  return Serial.read();
}

uint32_t last_frame_start = 0;

void loop() {
  Serial.flush();
wait_for_player_connect:
  /* Wait until we get byte 'P' from player */
  while (Serial.available() < 1);
  uint8_t player_msg = Serial.read();
  if (player_msg != 'P') {
    goto wait_for_player_connect;
  }
  /* send start byte to player */
  Serial.write(0x01);
  /* now we can start playing */
  int bytes_read;
  frame_timer = 0;
loop_start:
  if (Serial.available() < MAX_FRAME_BYTES) {
    Serial.write(MAX_FRAME_BYTES - Serial.available());
  }
  long frame_start;
  while (frame_timer < 157) { frame_start = micros(); }
  // Serial.println(frame_timer);
  frame_start = micros();
  // Serial.println(frame_start - last_frame_start);
  last_frame_start = frame_start;
  
  frame_timer = 0;
  uint8_t reg, val, byte1, byte2;
  long micro_delta = 0;
  long current_offset = 0;
  long current_delta = 0;
  bytes_read = 0;
read_next_tone:
  current_offset = micros() - frame_start;
  if (current_offset > MAX_FRAME_OFFSET_US) {
    goto loop_start;
  }
    
  byte1 = get_buffer_byte();
  byte2 = get_buffer_byte();
  bytes_read += 2;
  if (bytes_read == MAX_FRAME_BYTES) {
    /* cannot do more than this amount of bytes a frame */
    goto loop_start;
  }
  if (!(byte1 & 0x80)) {
    /* this is a time stamp or a frame start marker */
    micro_delta = ((uint16_t)byte1 << 8) | byte2;
    if (micro_delta == 0) {
      /* frame start marker! */
      goto loop_start;
    }
    /* time stamp! */
    current_delta = micros();
    while (current_delta < micro_delta) {
      current_delta = micros();
    }
    goto read_next_tone;
  }

  reg = byte1 & 0x0F;
  val = byte2;
  write_psg(reg, val);

  goto read_next_tone;
}
