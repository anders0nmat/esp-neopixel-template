#include <ESP8266WiFi.h>
#include "led_interface.hpp"
#include "wifi.credentials.h"


//#define serial_dbg

#ifdef serial_dbg
#define _debugline(x) (x)
#else
#define _debugline(x)
#endif

const char* ssid = CREDENTIALS_SSID; // Your WiFi SSID
const char* password = CREDENTIALS_PASSWORD; // Your WiFi Password

WiFiServer server(22255);
// Exactly 858 LEDs + 1 DEBUG LED (0) (For 3.3V -> 5V Signal) (visible LEDS at Index 1 to 857)
// LED Data-Line at Pin 2
// (Default-Value) max. 32 Animations
// (Default-Value) Animations are updated every 0.1s
NeoPixels<NeoGrbwFeature, NeoEsp8266AsyncUart1Sk6812Method> leds(858 + 1, 2); 

int lastSwitch;
unsigned long nextCheck;

void checkSwitch() {
	unsigned long currentTime = millis();
	if (currentTime > nextCheck) {
		if (digitalRead(3) != lastSwitch) {
			lastSwitch = digitalRead(3);
			_debugline(Serial.println("Status changed to: " + String(lastSwitch)));
			leds.toggle();
		}
		nextCheck = currentTime + 50;
	}
}

void processBinaryClient(WiFiClient& client) {
	if (client.peek() != static_cast<uint8_t>(CmdHeader::message_start)) return;
	std::vector<uint8_t> msg(client.available());
	client.read(msg.data(), msg.size() * sizeof(uint8_t));
	
	switch (leds.binaryCommand(msg)) {
	case CmdResult::status:
	case CmdResult::ok:
	case CmdResult::error:
	default: break;
	}
}

void acceptBinaryClient(WiFiClient& client) {
	if (!client) return;

	_debugline(Serial.println("New Client"));
	int del = 0;
	while (!client.available()) {
		if (del >= 500) {
			_debugline(Serial.print("Timeout, Client Disconnected"));	
			return;
		}
		++del;
		delay(5);
	}
	if (client.available()) 
		processBinaryClient(client);
	client.flush();
	_debugline(Serial.println("Client Disconnected"));
}

void setup() {
#ifdef serial_dbg
	Serial.begin(115200);
	while (!Serial);
	Serial.println();
	Serial.println();

	Serial.print("Connecting to WiFi ");
	Serial.println(ssid);
#endif

	WiFi.mode(WIFI_STA);
	WiFi.begin(ssid, password);

	while (WiFi.status() != WL_CONNECTED) {
		delay(500);
		_debugline(Serial.print("."));
	}
	_debugline(Serial.println("\nWiFi connected"));
	_debugline(Serial.println(WiFi.localIP()));

	server.begin();
	_debugline(Serial.println("Server started at port 22255"));

	leds.begin();
	_debugline(Serial.println("LEDs initialized"));

	pinMode(3, INPUT);
	lastSwitch = digitalRead(3);
	_debugline(Serial.println("Status of Switch: " + String(lastSwitch)));
	leds.toggle(lastSwitch == HIGH);
	_debugline(Serial.println("Running..."));
}

void loop() {
	checkSwitch();

	WiFiClient client = server.available();
	acceptBinaryClient(client);
	
	leds.update();
#ifdef serial_dbg
	const int dbg_led_count = 10;
	Serial.println("printing first " + String(dbg_led_count) + " LEDs");
	uint8_t * buf = strip.Pixels();
	for (int i = 0; i < dbg_led_count; i++) {
		byte r, g, b, w;
		g = *buf++;
		r = *buf++;
		b = *buf++;
		w = *buf++;
		Serial.printf("R: %d\t G: %d\t B: %d\t W: %d\t\n", r, g, b, w);

	}
	Serial.println("printing led values done");
#endif
}
