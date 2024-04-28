#include <Arduino.h>

#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecureBearSSL.h>

#include <ArduinoJson.h>
#include <ESPAsyncWebServer.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <DS3231.h>
#include <Wire.h>
#include <EEPROM.h>

#define REFRESH_DELAY_MS 60000

// ---- button/switch definitions

#define BUTTON D5

// ---- display definitions

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
// The pins for I2C are defined by the Wire-library. 
// On an ESP8266:       D2(SDA), D1(SCL)
#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C 
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

const int16_t up_char = 30;
const int16_t down_char = 31;

// ---- network definitions

const char* setup_ssid     = "StockTicker-AP";
const char* setup_password = "123456789";
IPAddress apIP;
AsyncWebServer server(80);

#define API_TOKEN "[insert your code here]" // get your key by going to https://www.finnhub.io/register

ESP8266WiFiMulti WiFiMulti;

const char* host = "finnhub.io";
const uint16_t port = 443;

bool setupMode = false;

#define MAX_STR_LEN 30 
struct ConfigurationObject {
  char networkName[MAX_STR_LEN];
  char networkPass[MAX_STR_LEN];
  char tickerName[MAX_STR_LEN];
};

ConfigurationObject currentConfiguration;

void notFound(AsyncWebServerRequest *request) {
    request->send(404, "text/plain", "Not found, try \\");
}

// display setup information
void displaySetupInformation(const char* ssid, const char* password, const char* ip) {
  // Clear the buffer
  display.clearDisplay();
  display.setTextSize(1);      // Normal 1:1 pixel scale
  display.setTextColor(SSD1306_WHITE); // Draw white text
  display.setCursor(2, 2);     // Start at top-left corner
  display.write("Setup mode");
  display.setCursor(2, 16);
  display.write("http://");
  display.write(ip);
  display.setCursor(2, 28);
  display.write(ssid);
  display.setCursor(2, 42);
  display.write(password);
  display.display();
}

void writeToEEPROM(String networkName, String networkPass, String tickerName) {
  ConfigurationObject newConfiguration;
  networkName.toCharArray(newConfiguration.networkName, MAX_STR_LEN);
  networkPass.toCharArray(newConfiguration.networkPass, MAX_STR_LEN);
  tickerName.toCharArray(newConfiguration.tickerName, MAX_STR_LEN);
  EEPROM.put(0, newConfiguration); 
  EEPROM.commit();  //write data from ram to flash memory. Do nothing if there are no changes to EEPROM data in ram
}

ConfigurationObject readConfigurationFromEEPROM() {
  ConfigurationObject configuration; 
  EEPROM.get(0, configuration); 
  configuration.networkName[MAX_STR_LEN - 1] = '\0';
  configuration.networkPass[MAX_STR_LEN - 1] = '\0';
  configuration.tickerName[MAX_STR_LEN - 1] = '\0';
  Serial.println("Configuration from EEPROM:");
  Serial.println(configuration.networkName);
  Serial.println(configuration.networkPass);
  Serial.println(configuration.tickerName);
  Serial.println("--------------------------");
  return configuration;
}


void setup_configuration_network() {
  
  Serial.println("Setting AP (Access Point)…");
  // Remove the password parameter, if you want the AP (Access Point) to be open
  WiFi.softAP(setup_ssid, setup_password);

  //IPAddress IP = WiFi.softAPIP();
  apIP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(apIP);

  // setup web server for configuration
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send(200, "text/html", 
          "<!DOCTYPE html>\n"
          "<html lang=\"en\">\n"
          "<head>\n"
          "    <meta charset=\"UTF-8\">\n"
          "    <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\n"
          "    <title>Ticker setup</title>\n"
          "</head>\n"
          "<body>\n"
          "    <h2>Ticker setup</h2>\n"
          "    <form id=\"myForm\" action=\"http://" + apIP.toString() + "/set\" >\n"
          "        <label for=\"net\">Network name:</label><br>\n"
          "        <input type=\"text\" id=\"net\" name=\"net\"><br><br>\n"
          "        <label for=\"pass\">Network password:</label><br>\n"
          "        <input type=\"text\" id=\"pass\" name=\"pass\"><br><br>\n"
          "        <label for=\"ticker\">Stock to track:</label><br>\n"
          "        <input type=\"text\" id=\"ticker\" name=\"ticker\" value=\"SPOT\"><br><br>\n"
          "        <button type=\"submit\">Submit</button>\n"
          "    </form>\n"
          "</body>\n"
          "</html>\n"
          );
    });

  server.on("/set", HTTP_GET, [] (AsyncWebServerRequest *request) {
      String networkName;
      String networkPass;
      String tickerName;
      if (request->hasParam("net")) {
        networkName = request->getParam("net")->value();
        Serial.println("New net is " + networkName);
      } 
      if (request->hasParam("pass")) {
        networkPass = request->getParam("pass")->value();            
        Serial.println("New pass is " + networkPass);
      } 
      if (request->hasParam("ticker")) {
        tickerName = request->getParam("ticker")->value();            
        Serial.println("New tickerName is " + tickerName);
      } 
      writeToEEPROM(networkName, networkPass, tickerName);
      request->send(200, "text/plain", "Updated succesfully - please restart ticker now.");
  });

  server.onNotFound(notFound);

  server.begin();
  
  displaySetupInformation(setup_ssid, setup_password, apIP.toString().c_str());
}

// display stock data
void displayStock(String name, String currentPrice, String ptChange) {
  // clear
  display.clearDisplay();
  // show message
  display.setTextSize(1);      // Normal 1:1 pixel scale
  display.setTextColor(SSD1306_WHITE); // Draw white text
  display.setCursor(2, 2);     // Start at top-left corner

  display.write(name.c_str());

  float float_pchange = ptChange.toFloat();
  int16_t change_char = up_char;
  if (float_pchange < 0) {
    Serial.println("Stock is down :(");
    change_char = down_char;
  } else {            
    Serial.println("Stock is up :)");
    change_char = up_char;
  }
  display.setCursor(2, 41);
  display.setTextSize(2);
  display.write(change_char);
  display.setTextSize(1);
  display.setCursor(16, 44);
  display.write(String(ptChange + "%").c_str());

  display.setCursor(2, 20);
  display.setTextSize(2);          
  display.write(String("$" + currentPrice).c_str());

  Serial.println("$"+currentPrice+", " + String(change_char + " " + ptChange+"%"));

  display.display();
}

void displayConnectingMessage(String networkName) {
    display.clearDisplay();
    display.setTextSize(1);      // Normal 1:1 pixel scale
    display.setTextColor(SSD1306_WHITE); // Draw white text  
    display.setCursor(2, 2);
    display.write("Connecting to:");
    display.setCursor(2, 16);
    display.write(networkName.c_str());
    display.display();

    Serial.println();
    Serial.println("Connecting to WiFi " + networkName);
}

void setup() {

  Serial.begin(115200);

  EEPROM.begin(sizeof(ConfigurationObject));

  // init setup switch and determine if in setup mode 
  pinMode(BUTTON, INPUT_PULLUP);
  digitalWrite(BUTTON, HIGH); //activate arduino internal pull up
  if (digitalRead(BUTTON)==LOW){
      Serial.println("Button is pressed entering setup mode");
      setupMode = true;
  }

  Serial.println("Initializing display");
  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }
  display.cp437(true);         // Use full 256 char 'Code Page 437' font
  display.clearDisplay();
  display.display();
  Serial.println("Done initializing display");
  // read configuration from EEPROM
  currentConfiguration = readConfigurationFromEEPROM();

  if (setupMode) {
    setup_configuration_network();
  } else {
    displayConnectingMessage(currentConfiguration.networkName);
    WiFi.mode(WIFI_STA);
    WiFiMulti.addAP(currentConfiguration.networkName, currentConfiguration.networkPass);    
  }
}

void loop() {

  if (setupMode) {
    Serial.print("AP IP address: ");
    Serial.println(apIP);
    Serial.println("Setup mode.");
    delay(1000);
    return;
  }

  // wait for WiFi connection
  if ((WiFiMulti.run() == WL_CONNECTED)) {

    std::unique_ptr<BearSSL::WiFiClientSecure> client(new BearSSL::WiFiClientSecure);

    //client->setFingerprint(fingerprint_sni_cloudflaressl_com);
    // Or, if you happy to ignore the SSL certificate, then use the following line instead:
    client->setInsecure();

    HTTPClient https;

    Serial.print("[HTTPS] begin...\n");
    if (https.begin(*client, host, port, "/api/v1/quote?symbol=" + String(currentConfiguration.tickerName) + "&token=" + API_TOKEN)) {  // HTTPS

      Serial.print("[HTTPS] GET...\n");
      // start connection and send HTTP header      
      int httpCode = https.GET();

      // httpCode will be negative on error
      if (httpCode > 0) {
        // HTTP header has been send and Server response header has been handled
        Serial.printf("[HTTPS] GET... code: %d\n", httpCode);

        // file found at server
        if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY) {
          String payload = https.getString();
          Serial.println(payload);

          DynamicJsonDocument doc(1024);
          deserializeJson(doc, payload);
          JsonObject obj = doc.as<JsonObject>();
          
          displayStock(currentConfiguration.tickerName, obj["c"].as<String>(), obj["dp"].as<String>());
        }
      } else {
        Serial.printf("[HTTPS] GET... failed, error: %s\n", https.errorToString(httpCode).c_str());
      }
      https.end();
    } else {
      Serial.printf("[HTTPS] Unable to connect\n");
    }
  } else {
    displayConnectingMessage(currentConfiguration.networkName);
  }

  Serial.println("Wait 60s before next round...");
  delay(REFRESH_DELAY_MS);
}

