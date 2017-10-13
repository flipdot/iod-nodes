/*
  It will reconnect to the server if the connection is lost using a blocking
  reconnect function. See the 'mqtt_reconnect_nonblocking' example for how to
  achieve the same result without blocking the main loop.


   Für Witty Cloud.
   Board: NodeMCU 1.0 ESP 12E

   Onboard LED blaues blinken: initiales WiFi Connect
   Grün: Kontakt zum Broker
   Rot: Kein Kontakt zum Broker

   auf der befehlszeile publishen:
   mosquitto_pub -h 192.168.178.81  -t 'aktoren/panel0' -m '4 1'

   und subscriben:
   mosquitto_sub -h 192.168.178.81  -t aktoren/panel0

   den Totmannkanal:
   mosquitto_sub -h 192.168.178.81  -t aliveTopic

*/

#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

const char* ssid = "security-by-iod";
const char* password = "45727545669727595";
const char* mqtt_server = "192.168.3.241";

const char* subscribeTopic = "actors/panel0";
//const char* publishTopic ="totmann/panel0";
const char* deviceName = "panel0";

// ----- definitions for WS2812 LED strings -----------------------------------------
#include <Adafruit_NeoPixel.h>
#ifdef __AVR__
#include <avr/power.h>
#endif

#define PIN 14

// Parameter 1 = number of pixels in strip
// Parameter 2 = Arduino pin number (most are valid)
// Parameter 3 = pixel type flags, add together as needed:
//   NEO_KHZ800  800 KHz bitstream (most NeoPixel products w/WS2812 LEDs)
//   NEO_KHZ400  400 KHz (classic 'v1' (not v2) FLORA pixels, WS2811 drivers)
//   NEO_GRB     Pixels are wired for GRB bitstream (most NeoPixel products)
//   NEO_RGB     Pixels are wired for RGB bitstream (v1 FLORA pixels, not v2)
//   NEO_RGBW    Pixels are wired for RGBW bitstream (NeoPixel RGBW products)
Adafruit_NeoPixel strip = Adafruit_NeoPixel(7, PIN, NEO_GRB + NEO_KHZ800);



WiFiClient espClient;
PubSubClient client(espClient);
long lastMsg = 0;
char msg[50];
int value = 0;

const int RED = 15;                 // buildt in RGB LED
const int BLUE = 13;
const int GREEN = 12;

void setup_wifi() {

  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    digitalWrite(BLUE, HIGH);
    delay(100);
    digitalWrite(BLUE, LOW);
    delay(400);
    Serial.print(".");
  }

  // randomSeed(micros());

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void callback(char* topic, byte* payload, unsigned int length) {

  StaticJsonBuffer<200> jsonBuffer;
  JsonObject& json_object = jsonBuffer.parseObject(payload);

  if (!json_object.success()) {
    Serial.println("parseObject() failed");
    Serial.print("Payload: ");
    for (int i = 0; i < length; i++)
      Serial.print((char) payload[i]);

    Serial.println();
    return;
  } else {
    Serial.println("Message arrived");
    json_object.prettyPrintTo(Serial);
    int led = json_object["led_num"];
    char red = json_object["red"];
    char green = json_object["green"];
    char blue = json_object["blue"];

    if (0 <= led && led < 5) {
      strip.setPixelColor(led, red, green, blue);
      strip.show();
    }
  }
}

void reconnect() {                                            // ToDo: if too many reconnect retrys, do reboot
  // Loop until we're reconnected                             // ESP.reset(); doesn't do the job, hard reset seems to be the way
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID                              // why random client ID?
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    if (client.connect(clientId.c_str())) {
      digitalWrite(GREEN, HIGH);
      digitalWrite(RED, LOW);
      Serial.println("connected");
      // Once connected, publish an announcement...
      // client.publish(publishTopic, deviceName);
      // ... and resubscribe
      client.subscribe(subscribeTopic);
    } else {
      digitalWrite(RED, HIGH);
      digitalWrite(GREEN, LOW);
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void setup() {
  pinMode(RED, OUTPUT);
  digitalWrite(RED, LOW);
  pinMode(GREEN, OUTPUT);
  digitalWrite(GREEN, LOW);
  pinMode(BLUE, OUTPUT);
  digitalWrite(BLUE, LOW);

  Serial.begin(115200);
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);

  strip.begin();
  strip.show();
  delay(1000);
  strip.setPixelColor(0, 0, 0, 2);
  strip.setPixelColor(1, 0, 0, 2);
  strip.setPixelColor(2, 0, 0, 2);
  strip.setPixelColor(3, 0, 0, 2);
  strip.setPixelColor(4, 0, 0, 2);
  strip.show();

}

void loop() {

  if (!client.connected()) {
    digitalWrite(RED, HIGH);
    digitalWrite(GREEN, LOW);
    reconnect();
  }
  client.loop();
  /*
    long now = millis();
    if (now - lastMsg > 2000) {
      lastMsg = now;
      ++value;
      // snprintf (msg, 75, "hello world #%ld", value);
      snprintf (msg, 75, deviceName);
      //Serial.print("Publish message: ");
      //Serial.println(msg);
      // client.publish(publishTopic, msg);
    }
  */
}
