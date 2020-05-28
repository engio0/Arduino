/*
 Author: Jeffrey Nelson <nelsonjm@macpod.net>
 Description: Very quickly written crappy code to read tachometer port data of the SX2 mini mill and likely the SC2 mini lathe.
 	for more information check http://macpod.net
 	This code was written on an 5v 16Mhz Arduino. I'm planning on writing 
 
 Tachometer Port Interface:
 	2 GND
 	2 5V+
 	1 LCDCS - Frame indicator,  pulled low during the transmission of a frame.
 	1 LCDCL - Clock line, pulled low when data should be read (we read on the falling edges)
 	1 LCDDI - Data line.
 
 Data information:
 	Reports if the spindel is stopped or running
 	Reports speed of the spindel in 10rpm increments
 
 Data format:
 	Every .75 seconds a packet is sent over the port.
 	Each packet consists of:
        (ONLY If the mill uses the new mill protocol): a 36 bit header
        Following this potential header are 4 frames. Each Frame consists of 17 bytes. 
        The first 8 bits represent an address, and the other bits represent data.
 
 	Frame 0: Represents 7-segment data used for 1000's place of rpm readout.
 		Address: 0xA0
 		Data: First bit is always 0
 			Next 7 bits indicate which of the 7-segments to light up.
 			Last bit is always 0
 	Frame 1: Represents 7-segment data used for 100's place of rpm readout.
 		Address: 0xA1
 		Data: First bit is always 0
 			Next 7 bits indicate which of the 7-segments to light up.
 			Last bit is always 0
 	Frame 2: Represents 7-segment data used for 10's place of rpm readout.
 		Address: 0xA2
 		Data: First bit is always 0
 			Next 7 bits indicate which of the 7-segments to light up.
 			Last bit is 1 if the spindel is not rotating, 0 otherwise.
 	Frame 3: Represents 7-segment data used for 1's place of rpm readout. This isn't used.
 		Address: 0xA3
 		Data: This is always 0x20
 
 
 
 
 7-segment display layout:
  d  
 c g
  f  
 b e
  a
 
 abcdefg
 00 = 1111101
 01 = 0000101
 02 = 1101011
 03 = 1001111
 04 = 0010111
 05 = 1011110
 06 = 1111110
 07 = 0001101
 08 = 1111111
 09 = 1011111
 
 */

#include <LiquidCrystal.h>
#include <avr/io.h>
#include <avr/interrupt.h>

#define LCDCS 2

#define LCDCL 3
#define LCDCL_INTERRUPT 1

#define LCDDI 4

#define LED_PIN 13

#define PACKET_BITS 68
// For newer mills there is a 36 bit header.
#define PACKET_BITS_HEADER 36

// Uncomment for newer mill protocol
#define PACKET_BITS_COUNT PACKET_BITS+PACKET_BITS_HEADER
// Uncomment for old mill protocol
//#define PACKET_BITS_COUNT PACKET_BITS

#define MAXCOUNT 503500


#define LCD_RS 5
#define LCD_EN 6
#define LCD_D4 7
#define LCD_D5 8
#define LCD_D6 9
#define LCD_D7 10


volatile uint8_t packet_bits[PACKET_BITS_COUNT];
volatile uint8_t packet_bits_pos;

LiquidCrystal lcd(LCD_RS, LCD_EN, LCD_D4, LCD_D5, LCD_D6, LCD_D7);

void setup() {
  Serial.begin(9600);
  pinMode(LCDCS, INPUT);
  pinMode(LCDCL, INPUT);
  pinMode(LCDDI, INPUT);
  pinMode(LED_PIN, OUTPUT);
  lcd.begin(16, 2);

  lcd.setCursor(0, 1);
  lcd.print("macpod.net");
  lcd.setCursor(0, 0);

  EIMSK |= (1<<INT1);  //Enable INT1

    //Trigger on falling edge of INT1
  EICRA |= (1<<ISC11);
  EICRA &= ~(1<<ISC10);

  TIMSK0 &= ~(1<<TOIE0); // Disable timer0
}



void loop() {
  int i;
  int rpm;
  unsigned long count;
  packet_bits_pos = 0;

  while (digitalRead(LCDCS) == LOW); // Wait to end of packet if we are in one.
  block_delay(227272); // ~200ms delay


  // Now we are ready to read a packet..
  EIMSK |= (1 << INT1);

  for (count = 0; packet_bits_pos < PACKET_BITS_COUNT && count < MAXCOUNT; count++) { // Loop until all bits are read or we timeout ~600ms
    asm("nop");
  }

  EIMSK &= ~(1 << INT1);

  if (packet_bits_pos == PACKET_BITS_COUNT && count < MAXCOUNT) {
    /* 
     // Debug stuff...
     print_bits(0, 8); // Register 0
     Serial.print(": ");
     print_bits(8, 9);
     Serial.print("\t");
     
     print_bits(17, 8); // Register 1
     Serial.print(": ");
     print_bits(25, 9);
     Serial.print("\t");    
     
     print_bits(34, 8); // Register 2
     Serial.print(": ");    
     print_bits(42, 9);
     Serial.print("\t");    
     
     print_bits(51, 8); // Register 3
     Serial.print(": ");    
     print_bits(59, 9);
     Serial.print("\n");
     */

  lcd.setCursor(0, 0);
    rpm = get_rpm();
    if (rpm == -1) {
      Serial.println("Communication error 2");
    } 
    else if (rpm == 0) {
      Serial.println("Stopped"); 
      lcd.print("Stopped");
    } 
    else {
      Serial.print(rpm, DEC);
      Serial.println(" rpm");
      lcd.print(rpm);
      lcd.print(" rpm");
    }
  } 
  else {
    Serial.println("Communication error 1");
  }


  for (packet_bits_pos = 0; packet_bits_pos < PACKET_BITS_COUNT; packet_bits_pos++) {
    packet_bits[packet_bits_pos] = 0;
  }
}


//----------------------------------------------------------------------------------------------------
// Assumes 68 bits were received properly. Returns the spindle speed or -1 if the values are absurd.
int get_rpm()
{
  int temp, ret = 0;
  if (build_address(0) != 0xA0) {
    return -1;
  }
  temp = get_digit_from_data(build_data(8));
  if (temp == -1) {
    return -1;
  }
  ret += temp*1000;

  if (build_address(17) != 0xA1) {
    return -1;
  } 
  temp = get_digit_from_data(build_data(25));
  if (temp == -1) {
    return -1;
  }
  ret += temp*100;  
  
  if (build_address(34) != 0xA2) {
    return -1;
  } 
  temp = get_digit_from_data(build_data(42));
  if (temp == -1) {
    return -1;
  }
  ret += temp*10;


  if (build_address(51) != 0xA3) {
    return -1;
  }
  temp = build_data(59);
  if (temp != 0x20) {
    return -1; 
  }
  
  return ret;
}

// An address is 8 bits long
uint8_t build_address(uint8_t start_address)
{
  uint8_t ret = 0x1;
  uint8_t i;

  if (PACKET_BITS_COUNT != PACKET_BITS) {
    // Compensate for header
    start_address += PACKET_BITS_HEADER;
  }

  for (i = start_address; i < start_address + 8; i++) {
    ret = (ret << 1) ^ ((packet_bits[i] & B00010000) ? 1 : 0);
  }
  return ret;
}

// Data is 9 bits long
uint16_t build_data(uint8_t start_address)
{
  uint16_t ret = 0;
  uint8_t i;

  if (PACKET_BITS_COUNT != PACKET_BITS) {
    // Compensate for header
    start_address += PACKET_BITS_HEADER;
  }

  for (i = start_address; i < start_address + 9; i++) {
    ret = (ret << 1) ^ ((packet_bits[i] & B00010000) ? 1 : 0);
  }
  return ret;
}

int get_digit_from_data(uint16_t data)
{
  uint16_t segments = (data & 0xFE) >> 1;
  int ret = 0;
  switch(segments) {
  case 0x7D:
    ret = 0;
    break;
  case 0x05:
    ret = 1;
    break;
  case 0x6B:
    ret = 2;
    break;
  case 0x4F:
    ret = 3;
    break;
  case 0x17:
    ret = 4;
    break;
  case 0x5E:
    ret = 5;
    break;
  case 0x7E:
    ret = 6;
    break;
  case 0x0D:
    ret = 7;
    break;
  case 0x7F:
    ret = 8;
    break;
  case 0x5F:
    ret = 9;
    break;
  default:
    ret = -1;
    break;
  }
  return ret;
}

// Returns 1 if stopped, 0 otherwise.
uint8_t spindle_stopped(uint16_t data)
{
  return data & 0x1;
}


//-----------------------------------------------------------------------------------------------------



void print_bits(int start, int len) {
  if (PACKET_BITS_COUNT != PACKET_BITS) {
    // Compensate for header
    start += PACKET_BITS_HEADER;
  }

  for (int i = start; i < start+len; i++) {
    if (packet_bits[i] & B00010000) {
      Serial.print('1');
    } 
    else {
      Serial.print('0');
    }
  }
}



// 100000 ~= 88ms
void block_delay(unsigned long units)
{
  unsigned long i;

  for (i = 0; i < units; i++) {
    asm("nop"); // Stop optimizations
  }
}



SIGNAL(INT1_vect)
{
  packet_bits[packet_bits_pos] = PIND;
  packet_bits_pos++;
}
