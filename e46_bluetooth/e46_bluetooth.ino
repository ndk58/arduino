
// BMW E81/E87 KCAN BT hands free interface
// Written by Luca Forattini (07/02/2019)
// Freely useable for non commercial purposes as long as credit is given where credit is due!
// Suggested board is the Arduino Uno or mini
// CAN Bus lib by Cory Fowler https://github.com/coryjfowler/MCP_CAN_lib
// Filtered to read only 0x1D6 steering wheel commands
// The BT module (electrodragon.com 1743NRZ) to which I connected it has 5 pushbuttons connected to a resistor array
// it detects the function by checking the resistor (voltage drop to GND) so pins must be left floating individually
// else the function will fail to decode BT side (resistors are all connected together to pin 8 of the BT chip 
// 1743NRZ with P25Q80H 8Mb flash chip for custom voice prompts)
// https://www.aliexpress.com/item/5V-Wifi-Wireless-Bluetooth-Audio-Receiver-Board-Module-For-Automotive-Audio-With-Stereo-Amplifier-Headphone-USB/32809733834.html

// The module has 2 double function buttons, the next and prev track also double as increase and decrease Multimedia volume on the phone side if long pressed.
// To mimic this on the Arduino, I check how often the command is received. The 0x1D6 ID has a keep alive (0x0C C0) pulse every second 
// If you keep the next or prev button pressed down these commands will be sent in sequence (there is no pressed/depress ID just what command has been pressed)
// so I simply set a flag and timeout of the previous command and check if the next command is the same I execute else I zero the flag if the timeout is reached

// 27/09/2020
// I modified the PLAY routine so that keeping the play (answer/phone) button pressed doesn't trigger a double press which triggers the phone to call the last number
// This is due to the fact that the K-CAN does not report any button release codes as stated above. I used the above routine and added a specific check for the PLAY
// button. When the play button is pressed a flag is set and a timer is reset. When the button is released and the timer expires, if the PLAY flag was set the button
// is pressed.

/*  The MC2515 has 2 Mask registers and 6 Filters. The Mask tells the MCP which BITS the filters should check
 *  this must be fully understood to avoid filtering issues. Check the logic flow chart on the documentation.
 *  This mask will check the filters against ID bit 8 and ID bits 3-0.   
    MASK   + 0000 0000 0000 0000 =   0x010F0000

   When using standard frames (11bits) the remaining 18 bits are matched to the FIRST 2 DATA BYTES
   with the 2 most significant bits of those 18 left unused (11+2+16). This requires you to set the
   remaining unused bits to 0. -> 111 1 111 1 111 11[00 0000 0000 0000 0000]
    
   If there is an explicit filter match to those bits, the message will be passed to the
   receive buffer and the interrupt pin will be set.
   This example will NOT be exclusive to ONLY the above frame IDs, for that a mask such
   as the below would be used: 
    MASK   + 0000 0000 0000 0000 = 0x07FF0000
    
   This mask will check the filters against all ID bits and the first data byte:
    MASK   + 1111 1111 0000 0000 = 0x07FFFF00
   If you use this mask and do not touch the filters below, you will find that your first
   data byte must be 0x00 for the message to enter the receive buffer.
   
   At the moment, to disable a filter or mask, copy the value of a used filter or mask.
   
   Data bytes are ONLY checked when the MCP2515 is in 'MCP_STDEXT' mode via the begin
   function, otherwise ('MCP_STD') only the ID is checked.
***********************************************************************************/


#include <mcp_can.h>
#include <SPI.h>

unsigned long currentMillis;
unsigned long previousMillis;
long unsigned int rxId;
unsigned char len = 0;
unsigned char rxBuf[8];
int pressedpin = 0;
//int pressedplay = 0;

#define DEBUG           // comment out to remove serial debugging
#define DELAYCHECK 150  // in milliseconds - must be high enough for loop to process without interruption and low enough for command to complete

// define control button pins
#define MODE_PIN 9    // Speak button
#define PLAY_PIN 8    // Phone button
#define NEXT_PIN 7    // Up button
#define PREV_PIN 6    // Down button
#define EQ_PIN 5      // To Be Defined

#define CAN0_INT 2    // Set MCP INT output to pin 2
MCP_CAN CAN0(10);     // Set CS to pin 10

void setup()
{
  #ifdef DEBUG
  Serial.begin(115200);
  if(CAN0.begin(MCP_STDEXT, CAN_100KBPS, MCP_8MHZ) == CAN_OK) Serial.print("MCP2515 Init Okay!!\r\n");
  else Serial.print("MCP2515 Init Failed!!\r\n");
  #else
  CAN0.begin(MCP_STDEXT, CAN_100KBPS, MCP_8MHZ);
  #endif
  
  pinMode(MODE_PIN, INPUT);    // set pin mode as floating no pu
  pinMode(PLAY_PIN, INPUT);
  pinMode(NEXT_PIN, INPUT);
  pinMode(PREV_PIN, INPUT);
  pinMode(EQ_PIN, INPUT);
  pinMode(CAN0_INT, INPUT);    // set pin 2 for /INT input

  digitalWrite(MODE_PIN, LOW);    // Set floating (no pullups) all pins
  digitalWrite(PLAY_PIN, LOW);
  digitalWrite(NEXT_PIN, LOW);
  digitalWrite(PREV_PIN, LOW);
  digitalWrite(EQ_PIN, LOW);
                                        // 0000 0111 1111 11 11 0000 0000 0000 0000
  CAN0.init_Mask(0,0,0x07FF0000);                 // Init first mask... (these bits are checked 0x01D0-0x01DF)
  CAN0.init_Filt(0,0,0x01D60000);                 // Init first filter... 0x1D6
  //CAN0.init_Filt(1,0,0x01D60000);                 // Init second filter...
  
  CAN0.init_Mask(1,0,0x07FF0000);                // Init second mask... (yes this is needed as Filter Mask 1 is associated only with Filter 1 and 2)
  CAN0.init_Filt(2,0,0x01D60000);                // Init third filter...
  //CAN0.init_Filt(3,0,0x01D60000);                // Init fourth filter...
  //CAN0.init_Filt(4,0,0x01D60000);                // Init fifth filter...
  //CAN0.init_Filt(5,0,0x01D60000);                // Init sixth filter...
  
  CAN0.setMode(MCP_NORMAL);                // Change to normal mode to allow messages to be transmitted
}

void loop()
{
    currentMillis = millis();             // Update timer 
    if(!digitalRead(CAN0_INT))                    // If pin 2 is low, read receive buffer and decode command
    {
      CAN0.readMsgBuf(&rxId, &len, rxBuf); // Read data: len = data length, buf = data byte(s)
      #ifdef DEBUG
      Serial.print("ID: ");
      Serial.print(rxId, HEX);
      Serial.print(" Data: ");
      for(int i = 0; i<len; i++)           // Print each byte of the data
      {
        if(rxBuf[i] < 0x10)                // If data byte is less than 0x10, add a leading zero
        {
          Serial.print("0");
        }
        Serial.print(rxBuf[i], HEX);
        Serial.print(" ");
        
      }
      #endif
      
      if(rxBuf[0]==0xC1 && rxBuf[1]==0x0C) phone();         // Play/Pause/Reply/Call button PLAY_PIN
        else if(rxBuf[0]==0xC0 && rxBuf[1]==0x0D) voice();  // Switch modes on BT receiver (USB/BT/LINE IN)
        else if(rxBuf[0]==0xE0 && rxBuf[1]==0x0C) next();   // NEXT button pressed
        else if(rxBuf[0]==0xD0 && rxBuf[1]==0x0C) prev();   // PREV button pressed
      #ifdef DEBUG
        else if(rxBuf[0]==0xC8 && rxBuf[1]==0x0C) Serial.print("- Vol + Button Pressed ");
        else if(rxBuf[0]==0xC4 && rxBuf[1]==0x0C) Serial.print("- Vol - Button Pressed ");
        else if(rxBuf[0]==0xC0 && rxBuf[1]==0x1C) Serial.print("- Cycle CD/RADIO/AUX Button Pressed (no action on BT)");   // Cycle CD/RADIO/AUX (no action)
        else if(rxBuf[0]==0xC0 && rxBuf[1]==0x4C) Serial.print("- CD Button Pressed (no action on BT)");
        //else if(rxBuf[0]==0xC0 && rxBuf[1]==0x0C) pressedpin = 0; // Reset pressed pin flag for double function button
      Serial.println();
      #endif
    }
    
    if (currentMillis-previousMillis>DELAYCHECK) {      // Release next/prev/play buttons if no buttons pressed after delay
      previousMillis = currentMillis;                   // Reset delay counter      
      if (pressedpin==NEXT_PIN || pressedpin==PREV_PIN) {
        #ifdef DEBUG
        Serial.print("- Buttons Released ");
        #endif
        releasebuttons();     // Release all buttons
        pressedpin = 0;       // Reset flag
      } else if (pressedpin==PLAY_PIN) {                // If Play was pressed and released the delay has expired so execute play
        #ifdef DEBUG
        Serial.print("- Play Button Released so execute PLAY");
        #endif
        pressbutton(PLAY_PIN);
        pressedpin = 0;       // Reset flag
        }
      }
}

void longpressbutton(int pin)
{
  pressedpin = pin;           // save pressed function/pin
  pinMode(pin, OUTPUT);       // set pin as output to drive BT IC input
  digitalWrite(pin, LOW);     // press button (keep pressed until timeout)
  //delay (1200);
  //digitalWrite(pin, HIGH);
  //pinMode(pin, INPUT);      // High Z again
  //digitalWrite(pin, LOW);   // set pin floating no pullups
}

void releasebuttons(){          // release buttons
  digitalWrite(PREV_PIN, HIGH);
  digitalWrite(NEXT_PIN, HIGH);
  //digitalWrite(PLAY_PIN, HIGH);
  pinMode(PREV_PIN, INPUT);      // High Z again
  pinMode(NEXT_PIN, INPUT);      // High Z again
  //pinMode(PLAY_PIN, INPUT);      // High Z again
  digitalWrite(PREV_PIN, LOW);   // set pin floating no pullups
  digitalWrite(NEXT_PIN, LOW);   // set pin floating no pullups
  //digitalWrite(PLAY_PIN, LOW);   // set pin floating no pullups
}

void pressbutton(int pin)
{
  //Serial.print("- Press button Executed ");
  pressedpin = pin;         // save pressed function/pin
  pinMode(pin, OUTPUT);     // set pin as output to drive BT IC input
  digitalWrite(pin, LOW);   //  toggle switch
  delay (200);
  digitalWrite(pin, HIGH);
  delay (100);
  pinMode(pin, INPUT);      // High Z again
  digitalWrite(pin, LOW);   // set pin floating no pullups
}

void voice()
{
  Serial.print("- Voice/Mode Button Pressed (MODE)");
  //pressedpin = MODE_PIN;
  pressbutton(MODE_PIN);
}

void eq()
{
  Serial.print("- Mode Button Pressed ");
  //pressedpin = EQ_PIN;
  pressbutton(EQ_PIN);
}

void phone()
{
  Serial.print("- Phone Button Pressed (Play/Pause/Call/Answer)"); 
  pressedpin = PLAY_PIN;          // Set play flag (do not press play until button is RELEASED)
  previousMillis = currentMillis; // Reset delay timer if button is (still) pressed
  //pressbutton(PLAY_PIN);
}

void next()
{
  Serial.print("- UP Button Pressed down");
    longpressbutton(NEXT_PIN);  // push down
    //pressedpin = NEXT_PIN;      // Set flag 
    previousMillis = currentMillis; // Reset delay timer if button is pressed
    //previousMillis = millis();  // Reset delay check
}

void prev()
{
  Serial.print("- DOWN Button Pressed down");
    longpressbutton(PREV_PIN);  // push down
    //pressedpin = PREV_PIN;      // Set flag 
    previousMillis = currentMillis; // Reset delay timer if button is pressed
    //previousMillis = millis();  // Reset delay check
}
