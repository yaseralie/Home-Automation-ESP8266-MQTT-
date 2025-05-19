//Using ESP8266
//using library DHT sensor Library by Adafruit Version 1.4.3
//This program for send temp, humidity to mqtt broker
//Receive message from mqtt broker to turn on device
//Reference GPIO  https://randomnerdtutorials.com/esp8266-pinout-reference-gpios/

//Setup for MQTT and WiFi============================
#include <ESP8266WiFi.h>
//Library for MQTT:
#include <PubSubClient.h>
//Library for Json format using version 5:
#include <ArduinoJson.h>

//Setup for DHT======================================
#include <DHT.h>
#define DHTPIN 2  //GPIO2 atau D4
// Uncomment the type of sensor in use:
//#define DHTTYPE    DHT11     // DHT 11
#define DHTTYPE    DHT11     // DHT 22 (AM2302)
//#define DHTTYPE    DHT21     // DHT 21 (AM2301)
DHT dht(DHTPIN, DHTTYPE);

// current temperature & humidity, updated in loop()
float t = 0.0;
float h = 0.0;

//declare topic for publish message
const char* topic_pub = "Home1_Pub";
//declare topic for subscribe message
const char* topic_sub = "Home1_Sub";

// Update these with values suitable for your network.
const char* ssid = "Your WIFI SSID";
const char* password = "PASSWORD WIFI";
const char* mqtt_server = "MQTT IP ADDRESS"; //without port

//for output
int lamp1 = 16; //lamp for mqtt connected D0
int lamp2 = 5; //lamp for start indicator D1
int lamp3 = 4; //lamp for stop indicator D2
int fan = 0; //lamp for stop indicator D3

WiFiClient espClient;
PubSubClient client(espClient);
//char msg[50];

void setup_wifi() {
  delay(100);
  // We start by connecting to a WiFi network
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  randomSeed(micros());
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void callback(char* topic, byte* payload, unsigned int length)
{
  //Receiving message as subscriber
  Serial.print("Message arrived in topic: ");
  Serial.println(topic);
  String json_received;
  Serial.print("JSON Received:");
  for (int i = 0; i < length; i++) {
    json_received += ((char)payload[i]);
    //Serial.print((char)payload[i]);
  }
  Serial.println(json_received);
  //if receive ask status from node-red, send current status of lamps
  if (json_received == "status")
  {
    check_stat();
  }
  else
  {
    //Parse json
    StaticJsonBuffer<200> jsonBuffer;
    JsonObject& root = jsonBuffer.parseObject(json_received);

    //get json parsed value
    //sample of json: {"device":"lamp1","trigger":"on"}
    Serial.print("Command:");
    String device = root["device"];
    String trigger = root["trigger"];
    Serial.println("Turn " + trigger + " " + device);
    Serial.println("-----------------------");
    //Trigger device
    //Lamp1***************************
    if (device == "lamp1")
    {
      if (trigger == "on")
      {
        digitalWrite(lamp1, HIGH);
      }
      else
      {
        digitalWrite(lamp1, LOW);
      }
    }
    //Lamp2***************************
    if (device == "lamp2")
    {
      if (trigger == "on")
      {
        digitalWrite(lamp2, HIGH);
      }
      else
      {
        digitalWrite(lamp2, LOW);
      }
    }
    //Lamp3***************************
    if (device == "lamp3")
    {
      if (trigger == "on")
      {
        digitalWrite(lamp3, HIGH);
      }
      else
      {
        digitalWrite(lamp3, LOW);
      }
    }
    //Fan***************************
    if (device == "fan")
    {
      if (trigger == "on")
      {
        digitalWrite(fan, LOW);
      }
      else
      {
        digitalWrite(fan, HIGH);
      }
    }
  }
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected())
  {
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    //if you MQTT broker has clientID,username and password
    //please change following line to    if (client.connect(clientId,userName,passWord))
    if (client.connect(clientId.c_str()))
    {
      Serial.println("connected");
      //once connected to MQTT broker, subscribe command if any
      client.subscribe(topic_sub);
      check_stat();
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
  Serial.begin(115200);
  dht.begin();
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
  //subscribe topic
  client.subscribe(topic_sub);
  //setup pin output
  pinMode(lamp1, OUTPUT);
  pinMode(lamp2, OUTPUT);
  pinMode(lamp3, OUTPUT);
  pinMode(fan, OUTPUT);
  //Reset lamp, turn off all
  digitalWrite(lamp1, LOW);
  digitalWrite(lamp2, LOW);
  digitalWrite(lamp3, LOW);
  //turn off relay for fan
  digitalWrite(fan, HIGH);
  check_stat();
}

void check_stat()
{
  //check output status--------------------------------------------
  //This function will update lamp status to node-red
  StaticJsonBuffer<300> JSONbuffer;
  JsonObject& JSONencoder = JSONbuffer.createObject();
  bool stat_lamp1 = digitalRead(lamp1);
  bool stat_lamp2 = digitalRead(lamp2);
  bool stat_lamp3 = digitalRead(lamp3);
  bool stat_fan = digitalRead(fan);
  //lamp1==========================
  if (stat_lamp1 == true)
  {
    JSONencoder["lamp1"] = true;
  }
  else
  {
    JSONencoder["lamp1"] = false;
  }
  //lamp2==========================
  if (stat_lamp2 == true)
  {
    JSONencoder["lamp2"] = true;
  }
  else
  {
    JSONencoder["lamp2"] = false;
  }
  //lamp3==========================
  if (stat_lamp3 == true)
  {
    JSONencoder["lamp3"] = true;
  }
  else
  {
    JSONencoder["lamp3"] = false;
  }
  //fan==========================
  //relay using ground to activate
  if (stat_fan == false)
  {
    JSONencoder["fan"] = true;
  }
  else
  {
    JSONencoder["fan"] = false;
  }

  char JSONmessageBuffer[100];
  JSONencoder.printTo(JSONmessageBuffer, sizeof(JSONmessageBuffer));
  client.publish("Home1_Stat", JSONmessageBuffer);
}

void loop() {
  delay(2000);
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  //read DHT sensor, temp and humidity-------------------------------
  t = dht.readTemperature();
  h = dht.readHumidity();
  if ((isnan(t)) || (isnan(h)))
  {
    Serial.println("Failed to read from DHT sensor!");
  }
  else
  {
    StaticJsonBuffer<300> JSONbuffer;
    JsonObject& JSONencoder = JSONbuffer.createObject();

    JSONencoder["device"] = "ESP8266";
    JSONencoder["temperature"] = t;
    JSONencoder["humidity"] = h;

    //use it if you want to add array:
    //JsonArray& values = JSONencoder.createNestedArray("values");
    //values.add(20);
    //values.add(21);
    //values.add(23);

    char JSONmessageBuffer[100];
    JSONencoder.printTo(JSONmessageBuffer, sizeof(JSONmessageBuffer));
    Serial.println("Sending message to MQTT topic..");
    Serial.println(JSONmessageBuffer);

    if (client.publish(topic_pub, JSONmessageBuffer) == true) {
      Serial.println("Success sending message");
    } else {
      Serial.println("Error sending message");
    }
    Serial.println("-------------");
  }

}
