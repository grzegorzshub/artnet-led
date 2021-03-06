//******************************************************************************
#define FIRMWARE_VERSION "1.2.5"  //MAJOR.MINOR.PATCH more info on: http://semver.org
#define SERIAL_SPEED 115200
//#define PRODUCTION true         //uncoment to turn the serial debuging
//******************************************************************************

// Libraries
#include <ESP8266WiFi.h>          //https://github.com/esp8266/Arduino

//-------------------- OTA - Updates over the air ------------------------------
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
//   |--------------|-------|---------------|--|--|--|--|--|
//   ^              ^       ^               ^     ^
//   Sketch    OTA update   File system   EEPROM  WiFi config (SDK)

// ------------------------------ Credentials ----------------------------------
//create credentials.h file in src folder with ssid and pass formated like below:
// const char* wifi_ssid = "yournetworkssid";
// const char* wif_password = "password";
#include "credentials.h"  //ignored by git to keep your network details private

#include <Adafruit_NeoPixel.h>    // neopixel lib from: https://github.com/adafruit/Adafruit_NeoPixel.git

#ifdef Anthony
  // neopixel setup, individual settings: neo pixel type, neo pin, etc
  #define NUMPIXELS 1
  #define NEOPIXEL_PIN 12 //D6 on nodeMCU
  Adafruit_NeoPixel pixels = Adafruit_NeoPixel(NUMPIXELS, NEOPIXEL_PIN, NEO_RGB + NEO_KHZ800);
#endif

#ifdef Grzegorz
  // neopixel setup
  #define NUMPIXELS 1
  #define NEOPIXEL_PIN 12 //D6 on nodeMCU
  Adafruit_NeoPixel pixels = Adafruit_NeoPixel(NUMPIXELS, NEOPIXEL_PIN, NEO_GRB + NEO_KHZ800);
#endif

extern "C" {
#include "user_interface.h"  //NOTE needed for esp info
}

#include <ArtnetWifi.h>   //cloned from https://github.com/rstephan/ArtnetWifi.git
ArtnetWifi artnet;
//------------------- ArtNet -----------------------------
#define UNIVERSE 0         //Max MSP test patch 0, desk 1
#define UNIT_ID 1

void setup()
{
  // Initialize the Serial (debuging use only, not need in production)
  #ifndef PRODUCTION // Not in PRODUCTION
    Serial.begin(SERIAL_SPEED);
    // compiling and esp info
    Serial.println("\r\n---------------------------------------"); //NOTE \r\n - new line, return
    Serial.println("Compiled: " __DATE__ ", " __TIME__ ", " __VERSION__);
    Serial.print("Version: "); Serial.print(FIRMWARE_VERSION); Serial.println("   by Grzegorz & Anthony");
    Serial.println("---------------------------------------");
    Serial.println( F("\r\n--- ESP Info --- ") );
    Serial.print( F("Heap: ") ); Serial.println(system_get_free_heap_size()); //IDEA add code size and free ram info
    Serial.print( F("Boot Vers: ") ); Serial.println(system_get_boot_version());
    Serial.print( F("CPU: ") ); Serial.println(system_get_cpu_freq());
    Serial.print( F("SDK: ") ); Serial.println(system_get_sdk_version());
    Serial.print( F("Chip ID: ") ); Serial.println(system_get_chip_id());
    Serial.print( F("Flash ID: ") ); Serial.println(spi_flash_get_id());
    Serial.print( F("Flash Size: ") ); Serial.println(ESP.getFlashChipRealSize());
    Serial.print( F("Vcc: ") ); Serial.println(ESP.getVcc());
    Serial.println();
  #endif

  // Connecting to a WiFi network with static IP
  // ---------------------------------------------------------------------------
  // IP settings stored in creditentials file in format i.e:
  // #include "IPAddress.h" //add IPaddress class
  // IPAddress ip(192,168,1,101); //ip address of the unit
  // IPAddress gateway(192,168,1,1);
  // IPAddress subnet(255,255,255,0);

  WiFi.config(ip, gateway, subnet);

  WiFi.mode(WIFI_STA); // http://esp8266.github.io/Arduino/versions/2.1.0-rc1/doc/libraries.html
  #ifndef PRODUCTION // Not in PRODUCTION
    Serial.print("Connecting to ");
    Serial.print(wifi_ssid);
  #endif

  WiFi.begin(wifi_ssid, wif_password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    #ifndef PRODUCTION // Not in PRODUCTION
      Serial.print(".");
    #endif
  }

  #ifndef PRODUCTION // Not in PRODUCTION
    Serial.println("");
    Serial.print("Connected with IP: ");
    Serial.println(WiFi.localIP());
  #endif

  ArduinoOTA.setHostname("neo");

  //initialize neopixel
  pixels.begin();
  neopixelTest();

  //initialize artnet
  artnet.begin();

  // this will be called for each packet received
  artnet.setArtDmxCallback(onDmxFrame);

  initializeOTA();
}

void loop(){
  // Update over air (OTA)
  ArduinoOTA.handle();
  // we call the read function inside the loop
  artnet.read();
}

void neopixelTest(){

  #ifndef PRODUCTION // Not in PRODUCTION
    Serial.println("\n\rNeo pixel test:");
    Serial.print("RED ");
  #endif
  // pixels.Color takes RGB values, from 0,0,0 up to 255,255,255
  pixels.setPixelColor(0, pixels.Color(255, 0, 0));
  pixels.show(); // This sends the updated pixel color to the hardware.
  delay(1000);

  #ifndef PRODUCTION // Not in PRODUCTION
    Serial.print("GREEN ");
  #endif
  pixels.setPixelColor(0, pixels.Color(0, 255, 0));
  pixels.show(); // This sends the updated pixel color to the hardware.
  delay(1000);

  #ifndef PRODUCTION // Not in PRODUCTION
    Serial.print("BLUE ");
  #endif
  pixels.setPixelColor(0, pixels.Color(0, 0, 255));
  pixels.show(); // This sends the updated pixel color to the hardware.
  delay(1000);

  #ifndef PRODUCTION // Not in PRODUCTION
    Serial.println("OFF");
  #endif
  pixels.setPixelColor(0, pixels.Color(0, 0, 0)); // Moderately bright green color.
  pixels.show(); // This sends the updated pixel color to the hardware.
}

void onDmxFrame(uint16_t universe, uint16_t length, uint8_t sequence, uint8_t* data){

  if (universe == UNIVERSE) {
    // Serial.print("DMX: Univ: ");
    // Serial.print(universe, DEC);
    // Serial.print(", Seq: ");
    // Serial.print(sequence, DEC);
    // Serial.print(", Data (");
    // Serial.print(length, DEC);
    // Serial.println("): ");

    int red_color = data[0];
    int green_color = data[1];
    int blue_color = data[2];

    Serial.print("dmx red: "); Serial.println(red_color);
    Serial.print("dmx green: "); Serial.println(green_color);
    Serial.print("dmx blue: "); Serial.println(blue_color);

    pixels.setPixelColor(0, pixels.Color(red_color, green_color, blue_color));
    pixels.show(); // This sends the updated pixel color to the hardware.
  } //if
}

// Initialize o Esp8266 OTA
void initializeOTA() {
  // Port defaults to 8266
  // ArduinoOTA.setPort(8266);
  // Hostname defaults to esp8266-[ChipID]
  // ArduinoOTA.setHostname("myesp8266");
  // No authentication by default
  // ArduinoOTA.setPassword((const char *)"123");

  ArduinoOTA.onStart([]() {
  #ifndef PRODUCTION_SERIAL // Not in PRODUCTION
    Serial.println("* OTA: Start");
  #endif
  });
  ArduinoOTA.onEnd([]() {
  #ifndef PRODUCTION_SERIAL // Not in PRODUCTION
    Serial.println("\n*OTA: End");
  #endif
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
  #ifndef PRODUCTION_SERIAL // Not in PRODUCTION
    Serial.printf("*OTA: Progress: %u%%\r", (progress / (total / 100)));
  #endif
  });
  ArduinoOTA.onError([](ota_error_t error) {
  #ifndef PRODUCTION_SERIAL // Not in PRODUCTION
    Serial.printf("*OTA: Error[%u]: ", error);
  #endif
  if (error == OTA_AUTH_ERROR) {
    #ifndef PRODUCTION_SERIAL // Not in PRODUCTION
      Serial.println("Auth Failed");
    #endif
  }
  else if (error == OTA_BEGIN_ERROR) {
    #ifndef PRODUCTION_SERIAL // Not in PRODUCTION
      Serial.println("Begin Failed");
    #endif
  }
  else if (error == OTA_CONNECT_ERROR) {
    #ifndef PRODUCTION_SERIAL // Not in PRODUCTION
      Serial.println("Connect Failed");
    #endif
  }
  else if (error == OTA_RECEIVE_ERROR) {
    #ifndef PRODUCTION_SERIAL // Not in PRODUCTION
      Serial.println("Receive Failed");
    #endif
  }
  else if (error == OTA_END_ERROR) {
    #ifndef PRODUCTION_SERIAL // Not in PRODUCTION
      Serial.println("End Failed");
    #endif
  }
  });
ArduinoOTA.begin();
#ifndef PRODUCTION_SERIAL // Not in PRODUCTION
  Serial.println("OTA Ready");
#endif
}
