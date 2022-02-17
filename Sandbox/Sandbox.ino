
int pressedpin = 0;

unsigned long currentMillis;
unsigned long previousMillis;

// define control button pins
#define MODE_PIN 4
#define PLAY_PIN 7
#define NEXT_PIN 9
#define PREV_PIN 12
#define EQ_PIN 13

#define DELAYCHECK 150  // in milliseconds - must be high enough for loop to process without interruption and low enough for command to complete
  
void setup() {
  
  // define pin mode
  pinMode(MODE_PIN, INPUT);
  pinMode(PLAY_PIN, INPUT);
  pinMode(NEXT_PIN, INPUT);
  pinMode(PREV_PIN, INPUT);
  pinMode(EQ_PIN, INPUT);

  // init of the pins
  digitalWrite(MODE_PIN, LOW);
  digitalWrite(PLAY_PIN, LOW);
  digitalWrite(NEXT_PIN, LOW);
  digitalWrite(PREV_PIN, LOW);
  digitalWrite(EQ_PIN, LOW);    

  Serial.begin(9600);  
}

void loop() {
  currentMillis = millis();

  delay(5000);
  next();
  pressedpin=NEXT_PIN;
   Serial.println(currentMillis);
   Serial.println(previousMillis);
  if (currentMillis-previousMillis>DELAYCHECK) {      // Release next/prev/play buttons if no buttons pressed after delay
    previousMillis = currentMillis;                   // Reset delay counter      
    if (pressedpin==NEXT_PIN || pressedpin==PREV_PIN) {
      #ifdef DEBUG
      Serial.println("- Buttons Released ");
      #endif
      releasebuttons();     // Release all buttons
      pressedpin = 0;       // Reset flag
    } else if (pressedpin==PLAY_PIN) {                // If Play was pressed and released the delay has expired so execute play
      #ifdef DEBUG
      Serial.println("- Play Button Released so execute PLAY");
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

void releasebuttons(){          // release buttons
  Serial.println("Button releases");
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

void next()
{
  Serial.println("NEXT Button Pressed down");
    longpressbutton(NEXT_PIN);  // push down
    //pressedpin = NEXT_PIN;      // Set flag 
    previousMillis = currentMillis; // Reset delay timer if button is pressed
    //previousMillis = millis();  // Reset delay check
}

void prev()
{
  Serial.println("PREV Button Pressed down");
    longpressbutton(PREV_PIN);  // push down
    //pressedpin = PREV_PIN;      // Set flag 
    previousMillis = currentMillis; // Reset delay timer if button is pressed
    //previousMillis = millis();  // Reset delay check
}
