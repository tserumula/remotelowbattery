#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>
#include <ArduinoJson.h>

#define LED_D1_WIFI_ON 5 // Turns on when WIFI connected. Flashes when connection lost
#define LED_D2_B_LOW 4 // Turns on when battery Low

const char *wifi_ssid = "YOUR_WIFI_SSID"; //Replace with your values
const char *wifi_password = "YOUR_WIFI_PASSWORD"; //Replace with your values

// Provide the server Domain URL or Ip address below
const char *hostURL = "http://YOUR_SERVER_DOMAIN/PATH"; //Replace with your values

// Indicate battery-Low threshod below
const int BATTERY_LOW_THRESHOLD = 15; //15% is our battery Low level

unsigned long lastCheckTime = 0;
const unsigned long timerDelay = 10 * 60 * 1000; // 10 minutes
bool wifiConnected = false;
bool batteryLow = false;

void setup()
{

  pinMode(LED_D1_WIFI_ON, OUTPUT);  // set pin as output
  pinMode(LED_D2_B_LOW, OUTPUT); // set pin as output

  Serial.begin(9600); // Enable serial output
  delay(100);

  WiFi.begin(wifi_ssid, wifi_password); // Connect to WIFI

  Serial.println("Connecting");

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }

  Serial.print("Connected to WiFi network with IP Address: ");
  Serial.println(WiFi.localIP());
  
  wifiConnected = true;

}

// Our infinite Loop function for MCU
void loop()
{
  // Check if we have waited long enough for delay duration since last http connect
  
  long time_diff = millis() - lastCheckTime;

  //NOTE: We are using abs() function below because millis() will eventually overflow
  // back to zero after ~49 days

  if ( abs(time_diff) > timerDelay)
  {
    // Check WiFi connection status
    if (WiFi.status() == WL_CONNECTED)
    {
      WiFiClient client;
      HTTPClient http;

      wifiConnected = true;

      http.begin(client, hostURL);

      // Specify content-type header
      http.addHeader("Content-Type", "application/x-www-form-urlencoded");

      // Our POST data payload
      String httpRequestData = "auth=eyJ0eXAiOiJKV1QiLCJhbGciOiJIUzI1NiJ9&deviceId=esp8266_nmcu_12e";

      // Send HTTP POST request
      int responseCode = http.POST(httpRequestData);

      if (responseCode == 200)
      {
        String response = http.getString();
        Serial.println(response);

        // Parse JSON Response
        JsonDocument jsonDoc;

        // JSON input string.
        const char *json = response.c_str();

        // Deserialize the JSON document
        DeserializationError parseError = deserializeJson(jsonDoc, json);

        // Test if parsing succeeds.
        if (parseError)
        {
          Serial.print(F("deserializeJson() failed: "));
          Serial.println(parseError.f_str());
        }
        else
        {
          short int batteryLevel = jsonDoc["battery"];
          // Note : jsonDoc['battery'] above will return 0 if the value is not present in response 

          if( batteryLevel > 0 && batteryLevel < BATTERY_LOW_THRESHOLD ){
            
            batteryLow = true;

          }else{
            
            batteryLow = false;
          
          }
        }
      }
      else
      {
        Serial.print("Error code: ");
        Serial.println(responseCode);
      }
      // Free resources
      http.end();
    }
    else
    {
      wifiConnected = false;
      Serial.println("WiFi Disconnected");
    }

    lastCheckTime = millis();
  }

  if( batteryLow ){
    // Turn On battery Low LED indication
    digitalWrite(LED_D2_B_LOW, HIGH);

  }else{

    digitalWrite(LED_D2_B_LOW, LOW);
  
  }

  if( wifiConnected ){
    // Keep D1 LED ON
    digitalWrite(LED_D1_WIFI_ON, HIGH);
    
  }else{
    //Flash D1 LED
  
    digitalWrite(LED_D1_WIFI_ON, HIGH); 
    delay(1000);
    digitalWrite(LED_D1_WIFI_ON, LOW); 
  }

  delay(1000);

}