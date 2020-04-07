// Visual Micro is in vMicro>General>Tutorial Mode
// 
/*
    Name:       AirFilter.ino
    Created:	01/04/2020 11:55:42
    Author:     DESKTOP-DRHORI3\TOMGA
*/

// Define User Types below here or use a .h file
//


// Define Function Prototypes that use User Types below here or use a .h file
//


// Define Functions below here or use other .ino or cpp files
//

// The setup() function runs once each time the micro-controller starts
#include "PMS.h"
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include "DHTesp.h"
#include <gfxfont.h>
#include <Adafruit_SPITFT_Macros.h>
#include <Adafruit_SPITFT.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Wire.h>
#include <PubSubClient.h>

//Definintations
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 32 // OLED display height, in pixels
#define tachInputPIN  2            
#define pwmOutputPin 14             
#define pwmDuty 1024
#define calculationPeriod 1000 //Number of milliseconds over which to interrupts
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
DHTesp dht;
//Global Varibles
// Replace with your network credentials
const char* ssid = "Toogle";
const char* password = "ValeAbode42";
const int ESP_BUILTIN_LED = 2;
volatile int interruptCounter; //counter use to detect hall sensor in fan
int RPM;                      //variable used to store computed value of fan RPM
unsigned long previousmills; //Timing Varible for 
unsigned long speedmills;  //Timing Varible for 
unsigned long readmills;//Timing Varible for 
int count = 0;
float Celcius = 0;
float Fahrenheit = 0;
float rpmper = 0;
void ICACHE_RAM_ATTR handleInterrupt() { //This is the function called by the interrupt
	interruptCounter++;

}

void setup()
{
	
	WiFi.mode(WIFI_STA);
	WiFi.begin(ssid, password);
	while (WiFi.waitForConnectResult() != WL_CONNECTED) {
	//	Serial.println("Connection Failed! Rebooting...");
		delay(5000);
		ESP.restart();
	}
	ArduinoOTA.onStart([]() {
	//	Serial.println("Start");
	});
	ArduinoOTA.onEnd([]() {
	//	Serial.println("\nEnd");
	});
	ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
	//	Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
	});
	ArduinoOTA.onError([](ota_error_t error) {
	//	Serial.printf("Error[%u]: ", error);
	//	if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
	//	else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
	//	else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
	//	else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
	//	else if (error == OTA_END_ERROR) Serial.println("End Failed");
	});
	ArduinoOTA.begin();
	//Serial.println("Ready");
	//Serial.print("IP address: ");
	//Serial.println(WiFi.localIP());
	pinMode(ESP_BUILTIN_LED, OUTPUT);
	delay(2000);
	//sensors.begin();
	display.begin(SSD1306_SWITCHCAPVCC, 0x3C); // Address 0x3D for 128x64
	//pmsSerial.begin(9600);
	dht.setup(16, DHTesp::DHT22); // Connect DHT sensor to GPIO 16
	//Display IP on startup
	display.clearDisplay();
	display.setTextSize(1);
	display.setTextColor(WHITE);
	display.setCursor(0, 1);
	// Display static text
	display.println("IP");
	display.setTextSize(2);
	display.setTextColor(WHITE);
	display.setCursor(0, 10);
	display.println(WiFi.localIP());
	display.display();
	delay(2000);

	// initilise varibles
	previousmills = 0;
	speedmills = 0;
	readmills = 0;
	interruptCounter = 0;
	RPM = 0;
	
	//set fan pins
	pinMode(pwmOutputPin, OUTPUT);
	pinMode(tachInputPIN, INPUT_PULLUP);

	analogWriteFreq(25000); //sets the frequency required to controll the pwm fan 
	analogWrite(pwmOutputPin, pwmDuty);
	attachInterrupt(digitalPinToInterrupt(tachInputPIN), handleInterrupt, FALLING);
	Serial.begin(9600);
}

struct pms5003data {
	uint16_t framelen;
	uint16_t pm10_standard, pm25_standard, pm100_standard;
	uint16_t pm10_env, pm25_env, pm100_env;
	uint16_t particles_03um, particles_05um, particles_10um, particles_25um, particles_50um, particles_100um;
	uint16_t unused;
	uint16_t checksum;
};

struct pms5003data data;
int Temp= 0;
float humidity = 0;
float temperature = 0;
int pm25 = 0;
int pm100 = 0;
// Add the main program code into the continuous loop() function
void loop()
{
	ArduinoOTA.handle();
	int status = 1; //used to detect errors

		if (readPMSdata(&Serial)) {
			//on a sucseful read
			count = count + 1;
			if (count > 9) {
				count = 0;
			}
			pm25 = data.pm25_standard;
			pm100 = data.pm100_standard;
			status = 1;
		}
		else
		{
			//we end up here alot do nothing atm
			//status = 2;
			//Serial.end();
			//delay(500);
			//Serial.begin(9600);
		}
	
	//	humidity = dht.getHumidity();
			if ((millis() - readmills) > dht.getMinimumSamplingPeriod())
			{
				//only read this at min sampling otherise weird things can happne
				readmills = millis();
				temperature = dht.getTemperature();
				int i = 0;
				i = int(temperature);
				if (i < 100) {
					//good reading
					Temp = int(temperature);
				}
			}


	if ((millis() - previousmills) > calculationPeriod) {// Process counters once every second
		//update display 1 time a seoncd
		previousmills = millis();
		int count = interruptCounter;
		interruptCounter = 0;
		computeFanSpeed(count);
		updatedisplay(status);
		// sensor reads
		

	}

			analogWrite(pwmOutputPin, 1024); //curerntly just sets the fan to ~1000 rpm
	yield();
}

boolean readPMSdata(Stream *s) {
	//from example of PMS data decoded the stream from the sensor
	//removed serial outputs as only used serial port
	if (!s->available()) {
		return false;
	}

	// Read a byte at a time until we get to the special '0x42' start-byte
	if (s->peek() != 0x42) {
		s->read();
		return false;
	}

	// Now read all 32 bytes
	if (s->available() < 32) {
		return false;
	}

	uint8_t buffer[32];
	uint16_t sum = 0;
	s->readBytes(buffer, 32);

	// get checksum ready
	for (uint8_t i = 0; i < 30; i++) {
		sum += buffer[i];
	}

	for (uint8_t i=2; i<32; i++) {
//	  Serial.print("0x"); Serial.print(buffer[i], HEX); Serial.print(", ");
	}
	//Serial.println();
	

	// The data comes in endian'd, this solves it so it works on all platforms
	uint16_t buffer_u16[15];
	for (uint8_t i = 0; i < 15; i++) {
		buffer_u16[i] = buffer[2 + i * 2 + 1];
		buffer_u16[i] += (buffer[2 + i * 2] << 8);
	}

	// put it into a nice struct :)
	memcpy((void *)&data, (void *)buffer_u16, 30);

	if (sum != data.checksum) {
	//	Serial.println("Checksum failure");
		return false;
	}
	// success!
	return true;
	

}

void updatedisplay(int switchcase)
{
	// 1 for normal 2 for PMfail
	switch (switchcase) {
	case 1:
		//standard all working update
		display.clearDisplay();
		display.setTextSize(1);
		display.setTextColor(WHITE);
		display.setCursor(0, 1);
		// Display static text
		display.println("PM2.5");
		display.setTextSize(3);
		display.setTextColor(WHITE);
		display.setCursor(0, 10);
		display.println(pm25);
		//PM2.5
		//display.setCursor(0, 1);
		display.drawLine(50, 0, 50, 32, SSD1306_WHITE);
		//Temp
		display.setTextSize(1);
		display.setCursor(60, 0);
		display.println("C");
		display.setCursor(40, 0);
		display.println(count);
		display.setTextSize(3);
		display.setTextColor(WHITE);
		display.setCursor(53, 10);
		display.println(Temp);


		//pm10
		display.setTextSize(1);
		display.setTextColor(WHITE);
		display.setCursor(100, 1);
		// Display static text
		display.println("PM10");
		display.setTextSize(2);
		display.setTextColor(WHITE);
		display.setCursor(100, 9);
		display.println(pm100);


		display.setTextSize(1);
		display.setTextColor(WHITE);
		display.setCursor(85, 24);
		display.println("Fan");
		display.setCursor(105, 24);
		rpmper = (float(RPM) / 3000)*100;
		//rpmper = rpmper * 100;
		display.println(rpmper,0);
		display.setCursor(122, 24);
		display.println("%");
		display.display();
		break;
	case 2:
		// PM sensor fail
		display.clearDisplay();
		display.setTextSize(3);
		display.setTextColor(WHITE);
		display.setCursor(0, 1);
		// Display static text
		display.println("PM Fail");
		display.display();
		break;
	}
}
void setrpm()
{
	//placeholder

}


void computeFanSpeed(int count) {
	//interruptCounter counts 2 pulses per revolution of the fan over a one second period
	RPM = count / 2 * 60;
}
