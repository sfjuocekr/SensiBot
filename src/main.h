// Configuration and calibration goes in these:
#include "config.h"
#include "calibration.h"

// Normal includes
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <Thread.h>
#include <StaticThreadController.h>
#include <NTPClient.h>
#include <WiFiUdp.h>

WiFiClient wifiClient;
PubSubClient client(wifiClient);
Thread threadPublish = Thread();
StaticThreadController<1> threadController(&threadPublish);
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, time_server, TIMEZONE * 3600, 60000);

unsigned long lastReconnectAttempt = 0;

void callback(char *topic, byte *payload, unsigned int length) {}