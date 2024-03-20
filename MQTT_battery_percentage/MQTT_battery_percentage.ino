
//   Diagnostic test for the displayed colour order
//
// Written by Bodmer 17/2/19 for the TFT_eSPI library:
// https://github.com/Bodmer/TFT_eSPI

/* 
 Different hardware manufacturers use different colour order
 configurations at the hardware level.  This may result in
 incorrect colours being displayed.

 Incorrectly displayed colours could also be the result of
 using the wrong display driver in the library setup file.

 Typically displays have a control register (MADCTL) that can
 be used to set the Red Green Blue (RGB) colour order to RGB
 or BRG so that red and blue are swapped on the display.

 This control register is also used to manage the display
 rotation and coordinate mirroring. The control register
 typically has 8 bits, for the ILI9341 these are:

 Bit Function
 7   Mirror Y coordinate (row address order)
 6   Mirror X coordinate (column address order)
 5   Row/column exchange (for rotation)
 4   Refresh direction (top to bottom or bottom to top in portrait orientation)
 3   RGB order (swaps red and blue)
 2   Refresh direction (top to bottom or bottom to top in landscape orientation)
 1   Not used
 0   Not used

 The control register bits can be written with this example command sequence:
 
    tft.writecommand(TFT_MADCTL);
    tft.writedata(0x48);          // Bits 6 and 3 set
    
 0x48 is the default value for ILI9341 (0xA8 for ESP32 M5STACK)
 in rotation 0 orientation.
 
 Another control register can be used to "invert" colours,
 this swaps black and white as well as other colours (e.g.
 green to magenta, red to cyan, blue to yellow).
 
 To invert colours insert this line after tft.init() or tft.begin():

    tft.invertDisplay( invert ); // Where invert is true or false

*/
#include <ESP8266WiFi.h>
#include <PubSubClient.h>

#include <SPI.h>
#include <TFT_eSPI.h>       // Hardware-specific library

#define MIN_PERCENTAGE 0
#define MAX_PERCENTAGE 100
#define START_ANGLE 30
#define END_ANGLE 330

#define SLOW_DELAY 90
#define FAST_DELAY 10
#define LOADING_DELAY 10

#define GAGUE_WIDTH 20
#define LOADING_CIRCLE_WIDTH 20
#define LOADING_SCREEN_TIME 1000

//#include <Fonts/FreeMonoBoldOblique12pt7b.h>
TFT_eSPI tft = TFT_eSPI();  // Invoke custom library

uint screen_height = tft.height();
uint screen_width = tft.width();

uint angle, angle1, angle2, angle3, angle4;
uint p;
float delay_time;
float treshold_for_delay;

uint text_font_size = 3;
uint number_font_size = 10;
String percentage_text = "";
String loading_text = "analyzing";

uint rand_num;
uint loading_time;

////////////////////

const char* ssid = "KESO";
const char* password = "12345678";


const char* mqtt_server = "broker.emqx.io";
const unsigned int mqtt_server_port = 1883;
String clientId = "ESP8266Client-";
const char* topic1 = "toMQTT";
const char* topic2 = "fromMQTT";

WiFiClient espClient;
PubSubClient client(espClient);
unsigned long lastMsg = 0;

#define MSG_BUFFER_SIZE	(50)
char msg[MSG_BUFFER_SIZE];
int value = 0;

volatile  bool TopicArrived = false;
const     int mqttpayloadSize = 100;
char mqttpayload [mqttpayloadSize] = {'\0'};
String mqtttopic;

void setup_wifi() {

  delay(100);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    Serial.print(WiFi.status()); 
    Serial.print(' ');
  }

  randomSeed(micros());

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void callback(char* topic, byte* payload, unsigned int length) {
  if ( !TopicArrived )
  {
    memset( mqttpayload, '\0', mqttpayloadSize ); // clear payload char buffer
    mqtttopic = ""; //clear topic string buffer
    mqtttopic = topic; //store new topic
    memcpy( mqttpayload, payload, length );
    TopicArrived = true;
    Serial.print("Message arrived [");
    Serial.print(topic);
    Serial.print("]: ");
    Serial.print(mqttpayload);
    Serial.println();
  }
  /*
  receivedMsg = (char)payload[i]
  for (int i = 0; i < length; i++) {
    Serial.print(receivedMsg);
  }
  Serial.println();
  */
  
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    if (client.connect(clientId.c_str())) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      // client.publish(topic1, "hello world");
      // ... and resubscribe
      client.subscribe(topic2);
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}


void setup(void) {
  tft.init();
  tft.fillScreen(TFT_BLACK);
  tft.setRotation(1);
  tft.setTextDatum(CC_DATUM);

  Serial.begin(9600);

  mqtttopic.reserve(100);
  setup_wifi();
  client.setServer(mqtt_server, mqtt_server_port);
  client.setCallback(callback);
  
}

void loop() {

  if (!client.connected()) {
    reconnect();
  }
  client.loop();
  
  if ( TopicArrived )
  {
    loading_screen();
    tft.fillScreen(TFT_BLACK);
    TopicArrived = false;
    rand_num = atoi(mqttpayload);
    display_battery_charge(rand_num);
  }
  //rand_num = random(0, MAX_PERCENTAGE+1);
  //Serial.println(rand_num);
  //delay(3000);
}

void display_battery_charge(uint battery_level){
  tft.setTextSize(number_font_size);
  p = MIN_PERCENTAGE;
  delay_time = FAST_DELAY;
  treshold_for_delay = battery_level/2;
  while (battery_level >= p) {
    // DO CALCULATIONS
    if (p > MAX_PERCENTAGE) p = MIN_PERCENTAGE;
    
    percentage_text = String(p);
    angle = START_ANGLE + p * (END_ANGLE - START_ANGLE) / (MAX_PERCENTAGE-MIN_PERCENTAGE);
    
    // START DRAWING
    if (p == MIN_PERCENTAGE) {
      tft.fillScreen(TFT_BLACK);
      tft.drawSmoothArc(screen_width/2, screen_width/2, screen_width/2, screen_width/2 - 20, START_ANGLE, END_ANGLE, TFT_WHITE, TFT_BLACK, true);
      delay(500);
    }
    // Text
    tft.drawString(percentage_text, screen_width/2, screen_height/2);
    
    // Graphics
    if (angle > START_ANGLE) {
      tft.drawSmoothArc(screen_width/2, screen_width/2, screen_width/2, screen_width/2 - GAGUE_WIDTH, START_ANGLE, angle, TFT_GREEN, TFT_BLACK, true);
    }

    // adjusting delay time
    if (p > (treshold_for_delay)) {
      delay_time = FAST_DELAY + SLOW_DELAY*((float)(p-treshold_for_delay)/(battery_level-treshold_for_delay));
    }
    
    p += 1;
    delay(delay_time);
  }
}

void loading_screen(){
  tft.setTextSize(text_font_size);
  angle = 0;
  loading_time = 0;
  while (loading_time < LOADING_SCREEN_TIME) {

    // DO CALCULATIONS
    angle += LOADING_DELAY;
    angle = angle%360;
    angle1 = angle;
    angle2 = (angle + 90) % 360;
    angle3 = (angle + 180) % 360;
    angle4 = (angle + 270) % 360;

    // START DRAWING
    if (loading_time == 0){
      tft.fillScreen(TFT_BLACK);
    }
    // Text
    tft.drawString(loading_text, screen_width/2, screen_height/2);
    
    // Graphics
    tft.drawSmoothArc(screen_width/2, screen_width/2, screen_width/2, screen_width/2 - LOADING_CIRCLE_WIDTH, angle2, angle3, TFT_BLACK, TFT_BLACK, false);
    tft.drawSmoothArc(screen_width/2, screen_width/2, screen_width/2, screen_width/2 - LOADING_CIRCLE_WIDTH, angle4, angle1, TFT_BLACK, TFT_BLACK, false);
    tft.drawSmoothArc(screen_width/2, screen_width/2, screen_width/2, screen_width/2 - LOADING_CIRCLE_WIDTH, angle1, angle2, TFT_WHITE, TFT_BLACK, false);
    tft.drawSmoothArc(screen_width/2, screen_width/2, screen_width/2, screen_width/2 - LOADING_CIRCLE_WIDTH, angle3, angle4, TFT_WHITE, TFT_BLACK, false);
    
    loading_time += LOADING_DELAY;
    //Serial.println(loading_time);
    delay(LOADING_DELAY);
  }
}
