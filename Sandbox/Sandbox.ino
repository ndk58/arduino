
int pressedpin = 0;

// define control button pins
#define MODE_PIN 4
#define PLAY_PIN 7
#define NEXT_PIN 9
#define PREV_PIN 12
#define EQ_PIN 13
  
void setup() {
  
  // define pin mode
  pinMode(MODE_PIN, OUTPUT);
  pinMode(PLAY_PIN, OUTPUT);
  pinMode(NEXT_PIN, OUTPUT);
  pinMode(PREV_PIN, OUTPUT);
  pinMode(EQ_PIN, OUTPUT);

  // init of the pins
  digitalWrite(MODE_PIN, LOW);
  digitalWrite(PLAY_PIN, LOW);
  digitalWrite(NEXT_PIN, LOW);
  digitalWrite(PREV_PIN, LOW);
  digitalWrite(EQ_PIN, LOW);    

  Serial.begin(9600);  
}

void loop() {
  pressbutton(NEXT_PIN);
  delay(3000);
  //pressbutton(PREV_PIN);
  //delay(5000);
}

void pressbutton(int pin)
{
  Serial.println("Button pressed");
  pressedpin = pin;         // save pressed function/pin
  //pinMode(pin, OUTPUT);     // set pin as output to drive BT IC input
  digitalWrite(pin, HIGH);
  delay (800);
  //pinMode(pin, INPUT);      //
  digitalWrite(pin, LOW);   // S
  delay (800);
  digitalWrite(pin, HIGH);
  delay (800);
  //pinMode(pin, INPUT);      //
  digitalWrite(pin, LOW);   // S

}
