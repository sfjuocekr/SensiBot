#include "main.h"

// Define a name for this device family:
String clientId = "SensiBot";

// sensors here
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <BH1750.h>

const byte bme_address = 0x76;
const byte lux_address = 0x23;

Adafruit_BME280 myBME;
BH1750 myLUX(lux_address);
// end of sensor declaration

void threadPublishCallback() // Called every LOGPERIOD
{
  // Read sensor data at the start
  float _atmosTempC = myBME.readTemperature();
  float _atmosRH = myBME.readHumidity();
  float _atmosPSI = myBME.readPressure() * PSI_IN_PASCAL;
  float _alt = myBME.readAltitude(SEALEVELPRESSURE_HPA);
  float _lux = myLUX.readLightLevel();

  // Do something with the data
  Serial.print(F("threadPublishCallback: ") + String(millis() / 1000));
  Serial.print(F("\ttemperature: ") + String(_atmosTempC));
  client.publish(String(F("sensors/") + clientId + F("/temperature")).c_str(), String(_atmosTempC).c_str());
  Serial.print(F("\thumidity: ") + String(_atmosRH));
  client.publish(String(F("sensors/") + clientId + F("/humidity")).c_str(), String(_atmosRH).c_str());
  Serial.print(F("\tpressure: ") + String(_atmosPSI));
  client.publish(String(F("sensors/") + clientId + F("/pressure")).c_str(), String(_atmosPSI).c_str());
  Serial.print(F("\taltitude: ") + String(_alt));
  client.publish(String(F("sensors/") + clientId + F("/altitude")).c_str(), String(_alt).c_str());
  Serial.println(F("\tlux: ") + String(_lux));
  client.publish(String(F("sensors/") + clientId + F("/lux")).c_str(), String(_lux).c_str());
  
  // hier is je hack:
  String _payload = "{\"DeviceName\":";
  _payload += clientId;
  _payload += ",\"atmosTempC\":";
  _payload += _atmosTempC;
  _payload += ",\"atmosRH\":";
  _payload += _atmosRH;
  _payload += ",\"atmosPsi\":";
  _payload += _atmosPSI;
  _payload += ",\"altitude\":";
  _payload += _alt;
  _payload += ",\"lux\":";
  _payload += _lux;
  _payload += "}";

  client.publish(String(F("sensors/") + clientId + F("/json")).c_str(), _payload.c_str());
}

boolean reconnect() // Called when client is disconnected from the MQTT server
{
  if (client.connect(clientId.c_str()))
  {
    timeClient.update(); // Is this the right time to update the time, when we lost connection?

    Serial.print(F("reconnect: ") + String(millis() / 1000));
    Serial.println(F("\ttime: ") + String(timeClient.getEpochTime()));
    client.publish(String(F("sensors/") + clientId + F("/debug/connected")).c_str(), String(timeClient.getEpochTime()).c_str());

    threadPublishCallback(); // Publish after reconnecting, probably not necesary with small LOGPERIOD
  }

  return client.connected();
}

String generateClientIdFromMac() // Convert the WiFi MAC address to String
{
  byte _mac[6];
  String _output = "";

  WiFi.macAddress(_mac);

  for (int _i = 5; _i > 0; _i--)
  {
    _output += String(_mac[_i], HEX);
  }

  return _output;
}

void setup() // The usual suspects
{
  Serial.begin(115200);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(1000);
  }

  clientId += F("-") + generateClientIdFromMac();
  timeClient.begin();

  if (!myBME.begin(bme_address))
    Serial.println(F("BME"));
  if (!myLUX.begin(BH1750::CONTINUOUS_HIGH_RES_MODE))
    Serial.println(F("LUX"));

  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);

  threadPublish.enabled = true;
  threadPublish.setInterval(LOGPERIOD);
  threadPublish.onRun(threadPublishCallback);

  Serial.print(F("setup: ") + String(millis() / 1000));
  Serial.print(F("\tIP: "));
  Serial.print(WiFi.localIP());
  Serial.print(F("\tClientID: ") + clientId);
}

void loop() // Do not mess with the main loop!
{
  if (!client.connected())
  {
    unsigned long _now = millis();

    if (_now - lastReconnectAttempt > 5000) // try to reconnect every 5000 milliseconds
    {
      lastReconnectAttempt = _now;

      if (reconnect())
      {
        lastReconnectAttempt = 0;
      }
    }
  }
  else
  {
    client.loop();
  }

  // Run threads, this makes it all work on time!
  threadController.run();
}