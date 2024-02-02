#include <SPI.h>
#include <Ethernet.h>
#include <ArduinoJson.h>

// Device MAC address
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };

// Associate devices with pin names
// const int outputPins[5] = { 2, 3, 4, 5, 6 };

// Init Ethernet server
EthernetServer server(80);

void setup() {
  
  // Open serial communications and wait for port to open:
  Serial.begin(9600);

  // start the Ethernet connection and the server:
  Ethernet.begin(mac);

  // Check for Ethernet hardware present
  if (Ethernet.hardwareStatus() == EthernetNoHardware) {
    Serial.println("Ethernet shield was not found.  Sorry, can't run without hardware. :(");
    while (true) {
      delay(1);  // do nothing, no point running without Ethernet hardware
    }
  }

  // If Ethernet cable is not connected
  if (Ethernet.linkStatus() == LinkOFF) {
    Serial.println("Ethernet cable is not connected.");
  }

  // start the server
  server.begin();
  Serial.print("server is at ");
  Serial.println(Ethernet.localIP());
}


void loop() {

  // Listen for incoming clients
  EthernetClient client = server.available();

  // Define response and request objects
  JsonDocument res;
  JsonDocument req;

  if (client) {

    // Track if newLine
    boolean currentLineIsBlank = true;

    while (client.connected()) {
      if (client.available()) {

        char c = client.read();

        if (c == '\n' && currentLineIsBlank) {

          String jsonString = "";

          // Save Request body
          while (client.available()) {
            char bc = client.read();
            jsonString += bc;
          }

          // Create Json object from request string
          deserializeJson(req, jsonString);

          const char* action = req["action"];

          if (strcmp(action, "INITIALIZE") == 0) {
            int numberOfPins = sizeof(req["pins"]);
            for (int i = 0; i < numberOfPins; ++i) {
              pinMode(req["pins"][i], OUTPUT);
              digitalWrite(req["pins"][i], HIGH);
            }
            res["success"] = true;
          }

          if (strcmp(action, "CHECK-STATE") == 0) {
            res["state"] = digitalRead(req["device_pin"]);
            res["success"] = true;
          }

          if (strcmp(action, "CHANGE-STATE") == 0) {
            int pinState = digitalRead(req["device_pin"]);
            if (pinState == 0) {
              digitalWrite(req["device_pin"], HIGH);
              res["state"] = 1;
            } else {
              digitalWrite(req["device_pin"], LOW);
              res["state"] = 0;
            }
            res["success"] = true;
          }

          // Handle response
          printResponse(client, res);

          break;
        }

        // New line VS character handler
        if (c == '\n') {
          currentLineIsBlank = true;
        } else if (c != '\r') {
          currentLineIsBlank = false;
        }
      }
    }

    // Close connection
    client.stop();
  }
}

// Function to return HTTP response with propper headers
void printResponse(EthernetClient& client, JsonDocument& res) {

  // Headers
  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: application/json");
  client.println("Connection: close");
  client.println();

  // Print json to client
  serializeJson(res, client);
}