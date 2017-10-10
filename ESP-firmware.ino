#include <PubSubClient.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>

const char WiFiSSID[] = "security-by-iod";     //### your Router SSID
const char WiFiPSK[]  = "########"; //### your Router Password
uint8_t MAC_array[6];
char MAC_char[18];
IPAddress ip(192, 168, 42, 12); //Node static IP
IPAddress gateway(192,168,42,254);
IPAddress subnet(255, 255, 255, 0);
IPAddress mqtt_server(192,168,3,241);               //mqtt server ip
const int mqtt_port = 1883;                         //mqtt server port
const char* mqtt_topic = "sensors/all";        //mqtt topic
const char* mqtt_client_id = "IoT-Test-ESP8266";    //mqtt client id

WiFiClient wifi_client;
PubSubClient mqtt_client(mqtt_server, mqtt_port, wifi_client);

const int BLUE = 1;
const int shutdownPin = 0;   // positive pulse on gpio 0 tells Attiny to shut down ESP
const int switchState = 3;   // gpio 3 reads the state of the activator switch

char str[30];                 // publishing string
bool mqtt_con_state = false;  // mqtt connection state

StaticJsonBuffer<200> jsonBuffer;	// reserve 200Bytes for json-foo

ADC_MODE(ADC_VCC); // needed to use ESP.getVcc()

void blink(int count, bool long_blink){
  int blink_time = 10;
  if(long_blink) {
    blink_time = 50;
  }
  while(count-- > 0){
    digitalWrite(BLUE, LOW);
    delay(blink_time);
    digitalWrite(BLUE, HIGH);
    delay(200 - blink_time);  
  }
}

// ----- check if wifi connection is up -----------------------------------------
bool isConnected(long timeOutSec)
{
  timeOutSec = timeOutSec * 1000;
  int z = 0;
  while (WiFi.status() != WL_CONNECTED)
  {
    blink(1, 0);                   // blink green while wifi connection in process
    if (z == timeOutSec / 200)
    {
      return false;
    }
    z++;
  }
  mqtt_con_state = mqtt_client.connect(mqtt_client_id, mqtt_topic, 0, 0, "Connected");		
  return mqtt_con_state;
}

void setup() {

  pinMode(switchState, INPUT);  
  pinMode(BLUE, OUTPUT);
  pinMode(shutdownPin, OUTPUT);
  digitalWrite(BLUE, HIGH);           // blue LED off
  digitalWrite(shutdownPin, LOW);

  blink(1, 0);

  WiFi.config(ip, gateway, subnet);
  WiFi.mode(WIFI_STA);
  WiFi.begin(WiFiSSID, WiFiPSK);

  WiFi.macAddress(MAC_array);
  for (int i = 0; i < sizeof(MAC_array); ++i){
    sprintf(MAC_char, "%s%02x:", MAC_char, MAC_array[i]);
  }
  mqtt_client_id = MAC_char;

  isConnected(20);            // try 20 seconds to connect to wifi access point ..

}

/*
 * @return true if status was published, false if can't connect to broker
 **/
bool mqtt_publish(int status){
  if (mqtt_con_state) {
    sprintf(str, "status: %d", status);
	  
	JsonObject& json_object = jsonBuffer.createObject();
    json_object["esp-id"] = MAC_char;
	if(status == 1) {
        json_object["switch_closed"] = true;
	} else {
		json_object["switch_closed"] = false;
	}
	json_object["esp_voltage_mV"] = ESP.getVcc();
	
	char json_string[256];
	json_object.printTo(json_string, sizeof(json_string));
	  
    mqtt_client.publish(mqtt_topic, json_string); //Publish/send message to clound on outTopic    
    return true;
  } else {
    return false;
  }
}

void loop() {

  if (digitalRead(switchState) == 1)
  {
    blink(2, 1);
    mqtt_publish(1);
  }
  else
  {
    blink(1, 1);
    mqtt_publish(0);
  }
  
  mqtt_client.disconnect();

  blink(1, 0);
  
  digitalWrite(shutdownPin, HIGH);
  delay(100);  
  digitalWrite(shutdownPin, LOW);
  
  while(true)
  { 
    blink(1, 0);
  }
}
