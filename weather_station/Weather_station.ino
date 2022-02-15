#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <Arduino_JSON.h>
#include <ArduinoJson.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// WiFi credential
const char* ssid = "Paolino WiFi";
const char* pass = "Hekebico348dvk";

WiFiClient client;

// LED status
const int ESP_BUILTIN_LED = 2;

// Open Weather Map API server name
const char server[] = "api.openweathermap.org";

// Replace the next line to match your city and 2 letter country code
String nameOfCity = "Nuremberg,DE"; 
// How your nameOfCity variable would look like for Lagos on Nigeria
//String nameOfCity = "Lagos,NG"; 

// Replace the next line with your API Key
String apiKey = "4bb5c25772bcf79ccf2f76bd6e474241"; 

String text;

int jsonend = 0;
boolean startJson = false;
int status = WL_IDLE_STATUS;

int rainLed = 15;  // Indicates rain D8
int clearLed = 12; // Indicates clear sky or sunny D6
int snowLed = 14;  // Indicates snow D5
int hiTempLed = 13;  // Indicates red alert D7
int cloudsLed = 0;  // Indicates clouds D3

#define JSON_BUFF_DIMENSION 2500

unsigned long lastConnectionTime = 10 * 60 * 1000;  // last time you connected to the server, in milliseconds
const unsigned long postInterval = 20 * 60 * 1000;  // posting interval of 20 minutes  (20L * 1000L)

// display section
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);


void setup() 
{
  Serial.begin(115200);
  // weather part
  pinMode(clearLed, OUTPUT);
  pinMode(rainLed, OUTPUT);
  pinMode(snowLed, OUTPUT);
  pinMode(hiTempLed, OUTPUT);
  pinMode(cloudsLed, OUTPUT);
  //pinMode(cloudsLed, OUTPUT);

  // LED setup
  digitalWrite(hiTempLed, LOW);
  digitalWrite(rainLed, LOW);
  digitalWrite(snowLed, LOW);
  digitalWrite(clearLed, LOW);
  digitalWrite(cloudsLed, LOW);

  // Built-in LED is inverted. HIGH means OFF
  digitalWrite(ESP_BUILTIN_LED, HIGH);

  // display setup
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Address 0x3D for 128x64
    Serial.println(F("SSD1306 allocation failed"));
    for(;;);
  }
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 10);

  delay(2000);
  display.clearDisplay();

  Serial.println("Booting");
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, pass);
  Serial.println("connecting");
  while (WiFi.waitForConnectResult() != WL_CONNECTED) 
  {
    Serial.println("Connection Failed! Rebooting...");
    delay(5000);
    ESP.restart();
  }
  Serial.println("WiFi Connected");
  display.println("WiFi Connected");
  display.display(); 
  //printWiFiStatus(); 

  // Port defaults to 8266
  // ArduinoOTA.setPort(8266);

  // Hostname defaults to esp8266-[ChipID]
  // ArduinoOTA.setHostname("myesp8266");

  // No authentication by default
  // ArduinoOTA.setPassword((const char *)"123");

  ArduinoOTA.onStart([]() 
  {
    Serial.println("Start");
  });
  ArduinoOTA.onEnd([]() 
  {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) 
  {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) 
  {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });
  ArduinoOTA.begin();
  //Serial.println("Ready");
  //Serial.print("IP address: ");
  //Serial.println(WiFi.localIP());
  pinMode(ESP_BUILTIN_LED, OUTPUT);
 
  text.reserve(JSON_BUFF_DIMENSION);
  
}

void loop() {
  unsigned long temp;
  ArduinoOTA.handle();
  delay(2000);
 
  if (millis() - lastConnectionTime > postInterval) {
    // note the time that the connection was made:
    lastConnectionTime = millis();
    makehttpRequest();
  }  
}


// print Wifi status
void printWiFiStatus() {
  // print the SSID of the network you're attached to:
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  // print your WiFi shield's IP address:
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);

  // print the received signal strength:
  long rssi = WiFi.RSSI();
  Serial.print("signal strength (RSSI):");
  Serial.print(rssi);
  Serial.println(" dBm");
}

// to request data from OWM
void makehttpRequest() {
  // close any connection before send a new request to allow client make connection to server
  client.stop();
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 10);
  // if there's a successful connection:
  if (client.connect(server, 80)) {
    Serial.println("Sending the request");
    display.println("Updating the weather");
    display.display();
    // send the HTTP PUT request:
    client.println("GET /data/2.5/forecast?q=" + nameOfCity + "&APPID=" + apiKey + "&mode=json&units=metric&cnt=2 HTTP/1.1");
    client.println("Host: api.openweathermap.org");
    client.println("User-Agent: ArduinoWiFi/1.1");
    client.println("Connection: close");
    client.println();
    
    unsigned long timeout = millis();
    while (client.available() == 0) {
      if (millis() - timeout > 5000) {
        Serial.println(">>> Client Timeout !");
        client.stop();
        return;
      }
    }
    
    char c = 0;
    while (client.available()) {
      c = client.read();
      // since json contains equal number of open and close curly brackets, this means we can determine when a json is completely received  by counting
      // the open and close occurences,
      if (c == '{') {
        startJson = true;         // set startJson true to indicate json message has started
        jsonend++;
      }
      if (c == '}') {
        jsonend--;
      }
      if (startJson == true) {
        text += c;
      }
      //Serial.print("\n");
      //Serial.print("jsonend = ");
      //Serial.println(jsonend);
      //Serial.print("startJson = ");
      //Serial.println(startJson);
      //Serial.print("client.available = ");
      delay(50);
      //Serial.println(client.available());
      //Serial.print(text);
      // if jsonend = 0 then we have have received equal number of curly braces 
      if (jsonend == 0 && startJson == true) {
        //Serial.println("into the if");
        parseJson(text.c_str());  // parse c string text in parseJson function
        text = "";                // clear text string for the next time
        startJson = false;        // set startJson to false to indicate that a new message has not yet started
      }
    }
  }
  else {
    // if no connction was made:
    Serial.println("connection failed");
    return;
  }
}

//to parse json data recieved from OWM
void parseJson(const char * jsonString) {
  //StaticJsonBuffer<4000> jsonBuffer;
  //const size_t bufferSize = 2*JSON_ARRAY_SIZE(1) + JSON_ARRAY_SIZE(2) + 4*JSON_OBJECT_SIZE(1) + 3*JSON_OBJECT_SIZE(2) + 3*JSON_OBJECT_SIZE(4) + JSON_OBJECT_SIZE(5) + 2*JSON_OBJECT_SIZE(7) + 2*JSON_OBJECT_SIZE(8) + 720;
  //DynamicJsonBuffer jsonBuffer(bufferSize);
  //Serial.println("into parseJson 1");

  DynamicJsonDocument doc(4096);

  // FIND FIELDS IN JSON TREE
  DeserializationError error = deserializeJson(doc, jsonString);
  if (error) {
      Serial.print(F("deserializeJson() failed with code "));
      Serial.println(error.c_str());
      return;
  }  

  serializeJson(doc, Serial);

  JsonArray list = doc["list"];
  JsonObject nowT = list[0];
  JsonObject later = list[1];

  //Serial.println("into parseJson 2");

  // including temperature and humidity for those who may wish to hack it in
  
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 5);
  
  String city = doc["city"]["name"];
  Serial.print("\n");
  Serial.println(city);

  float tempNow = nowT["main"]["temp"];
  float humidityNow = nowT["main"]["humidity"];
  String weatherNow = nowT["weather"][0]["description"];
  //Serial.println(tempNow);
  //Serial.println(humidityNow);
  //Serial.println(weatherNow);
  display.print("Current Temp: ");
  display.print(tempNow);
  //display.print("°C");

  display.setCursor(0, 15);
  display.print("Now: ");
  display.print(weatherNow);  


  float tempLater = later["main"]["temp"];
  float humidityLater = later["main"]["humidity"];
  String weatherLater = later["weather"][0]["description"];
  //Serial.println(tempLater);
  //Serial.println(humidityLater);
  Serial.println(weatherLater);

  display.setCursor(0, 35);

  display.print("Later Temp: ");
  display.print(tempLater);
  //display.print("°C");

  display.setCursor(0, 45);
  display.print("Later: ");
  display.print(weatherLater);  
  
  display.display();

  //for debug
  //weatherLater = "rain";

  // checking for four main weather possibilities
  weatherLed(weatherNow, weatherLater, "clear");
  weatherLed(weatherNow, weatherLater, "rain");
  weatherLed(weatherNow, weatherLater, "snow");
  //weatherLed(weatherNow, weatherLater, "hail");
  weatherLed(weatherNow, weatherLater, "cloud");

  //for debug
  //tempLater = 29;  
  temperatureLed(tempNow, tempLater);

}

void temperatureLed(float Tnow, float Tlater) {
  if ((Tnow > 29) || (Tnow < 1))
    digitalWrite(hiTempLed,HIGH);
  else
    digitalWrite(hiTempLed,LOW);
  
  Serial.print("Tnow = ");
  Serial.print(Tnow);
  
}


// switch ON the weather led
void weatherLed(String nowT, String later, String weatherType) {
  int indexNow = nowT.indexOf(weatherType);
  int indexLater = later.indexOf(weatherType);
  int brokenclouds = later.indexOf("broken");
  int scatteredclouds = later.indexOf("scattered");

  // if weather type = rain, if the current weather does not contain the weather type and the later message does, send notification
  if (weatherType == "rain") { 
    if (indexLater != -1) {
      digitalWrite(rainLed,HIGH);
      digitalWrite(clearLed,LOW);
      digitalWrite(snowLed,LOW);
      digitalWrite(hiTempLed,LOW);
      digitalWrite(cloudsLed,LOW);
      Serial.println("Oh no! It is going to " + weatherType + " later! Predicted " + later);
    }
  }
  // for snow
  else if (weatherType == "snow") {
    if (indexLater != -1) {
      digitalWrite(snowLed,HIGH);
      digitalWrite(clearLed,LOW);
      digitalWrite(rainLed,LOW);
      digitalWrite(hiTempLed,LOW);
      digitalWrite(cloudsLed,LOW);
      Serial.println("Oh no! It is going to " + weatherType + " later! Predicted " + later);
    }    
  }
  else if (weatherType == "hail") { 
    if (indexLater != -1) {
      digitalWrite(hiTempLed,HIGH);
      digitalWrite(clearLed,LOW);
      digitalWrite(rainLed,LOW);
      digitalWrite(snowLed,LOW);
      digitalWrite(cloudsLed,LOW);
      Serial.println("Oh no! It is going to " + weatherType + " later! Predicted " + later);
    }
  }
  else if (weatherType == "clear") { 
    if (indexLater != -1) {
      digitalWrite(hiTempLed,LOW);
      digitalWrite(clearLed,HIGH);
      digitalWrite(rainLed,LOW);
      digitalWrite(snowLed,LOW);
      digitalWrite(cloudsLed,LOW);
      Serial.println("Oh no! It is going to " + weatherType + " later! Predicted " + later);
    }
  }
  else if (weatherType == "cloud") { 
    // I want broken/scattered clouds to show green and not yellow
    if ((brokenclouds != -1) || (scatteredclouds != -1)) {
      digitalWrite(hiTempLed,LOW);
      digitalWrite(clearLed,HIGH);
      digitalWrite(rainLed,LOW);
      digitalWrite(snowLed,LOW);
      digitalWrite(cloudsLed,LOW);
      Serial.println("Oh no! It is going to " + weatherType + " later! Predicted " + later);
    }
    else if (indexLater != -1) {
      digitalWrite(hiTempLed,LOW);
      digitalWrite(clearLed,LOW);
      digitalWrite(rainLed,LOW);
      digitalWrite(snowLed,LOW);
      digitalWrite(cloudsLed,HIGH);
      Serial.println("Oh no! It is going to " + weatherType + " later! Predicted " + later);
    }
  }
  // for clear sky, if the current weather does not contain the word clear and the later message does, send notification that it will be sunny later
  //else {
  //  if (indexNow == -1 && indexLater != -1) {
  //    Serial.println("It is going to be sunny later! Predicted " + later);
  //    digitalWrite(clearLed,HIGH);
  //    digitalWrite(rainLed,LOW);
  //    digitalWrite(snowLed,LOW);
  //    digitalWrite(hiTempLed,LOW);
  //  }
}
