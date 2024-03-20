#include <ESP8266WiFi.h>
#include <PubSubClient.h>

#include <FastLED.h>
#define DATA_PIN 12
#define CIRCLE_LEDS 12
#define NUM_LEDS 36
#define NUMBER_OF_RINGS 3

CRGB leds[NUM_LEDS];
int count = 0;
const unsigned int led_speed = 100; //lower number -> higher speed

int clockwise[4] = {0, 1, 2, 3};
int counter_clockwise[4] = {0, 1, 2, 3};
char* led_arr = "000";
// Update these with values suitable for your network.

//const char* ssid = "FIWARE";
//const char* password = "!FIWARE!on!air!";

//const char* ssid = "o2-HB3-6742-CG";
//const char* password = "88TCX9ADGG47Y89D";

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

void setup() {
  mqtttopic.reserve(100);
  //pinMode(BUILTIN_LED, OUTPUT);     // Initialize the BUILTIN_LED pin as an output
  Serial.begin(9600);
  setup_wifi();
  client.setServer(mqtt_server, mqtt_server_port);
  client.setCallback(callback);
  
  FastLED.addLeds<NEOPIXEL, DATA_PIN>(leds, NUM_LEDS);  // GRB ordering is assumed
}

void loop() {

  unsigned long start = millis();
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
  


  if ( TopicArrived )
  {
    TopicArrived = false;
    led_arr = mqttpayload;
    FastLED.clear(true);
  }

  for (int ring = 0; ring < NUMBER_OF_RINGS; ring++) {
    if (led_arr[ring] == '1'){
      set_ring(ring, clockwise);
    } else if (led_arr[ring] == '2') {
      set_ring(ring, counter_clockwise);
    }
  }
  
  FastLED.show();
  increment_rotation(clockwise, true);
  increment_rotation(counter_clockwise, false);

  unsigned long end = millis();
  Serial.println(end-start);  
  if (end-start<led_speed) FastLED.delay(led_speed-(end-start));
    
  
  //unsigned long now = millis();
  /*
  if (start - lastMsg > 2000) {
    lastMsg = start;
    ++value;
    snprintf (msg, MSG_BUFFER_SIZE, "hello world #%ld", value);
    Serial.print("Publish message: ");
    Serial.println(msg);
    client.publish("toMQTT", msg);
  }
  */
}

void set_ring(int ring_num, int rotation[]) {
  leds[rotation[0]+ring_num*CIRCLE_LEDS].setRGB(0, 0, 0);
  leds[rotation[1]+ring_num*CIRCLE_LEDS].setRGB(50, 0, 0);
  leds[rotation[2]+ring_num*CIRCLE_LEDS].setRGB(120, 0, 0);
  leds[rotation[3]+ring_num*CIRCLE_LEDS].setRGB(200, 0, 0);
}

void increment_rotation(int rotation[], bool clockwise_flag) {

  rotation[0] = rotation[1];
  rotation[1] = rotation[2];
  rotation[2] = rotation[3];
  if (clockwise_flag == true) {
    rotation[3]++;
  } else {
    rotation[3]--;
  }
  
  if (rotation[3] < 0) {
    rotation[3] = CIRCLE_LEDS-1;
  } else if (rotation[3] >= CIRCLE_LEDS) {
    rotation[3] = 0;
  }
}