#include <painlessMesh.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecure.h>

#define   MESH_PREFIX     "alertMesh"
#define   MESH_PASSWORD   "alertPassword"
#define   MESH_PORT       5555

#define   STATION_SSID     "YOUR_WIFI_SSID"
#define   STATION_PASSWORD "YOUR_WIFI_PWD"

// Configuration for the custom Node.js API server
// Enter the local IP address of the computer running your server.js API
#define API_HOST "192.168.X.XXX" // e.g., "192.168.1.100"
#define API_PORT 3000

Scheduler userScheduler;
painlessMesh  mesh;

void sendToFirebase(uint32_t nodeId) {
  if (WiFi.status() == WL_CONNECTED) {
    WiFiClient client; // Since the API runs locally, we can just use HTTP.

    HTTPClient http;
    // We send payload to our custom Node.js API
    String url = "http://" + String(API_HOST) + ":" + String(API_PORT) + "/api/alerts";
    
    http.begin(client, url);
    http.addHeader("Content-Type", "application/json");

    // Construct simple JSON payload for our API
    String payload = "{";
    payload += "\"nodeId\":" + String(nodeId) + ",";
    payload += "\"status\":\"ALERT\"";
    payload += "}";

    int httpCode = http.POST(payload);
    
    if (httpCode > 0) {
      Serial.printf("API POST successful, status code: %d\n", httpCode);
    } else {
      Serial.printf("API POST failed, error: %s\n", http.errorToString(httpCode).c_str());
    }
    http.end();
  } else {
    Serial.println("WiFi not connected. Wait for connection before sending.");
  }
}

void receivedCallback(uint32_t from, String &msg) {
  Serial.printf("Received from %u msg=%s\n", from, msg.c_str());
  
  // Basic substring check ensures lightweight message processing
  if (msg.indexOf("\"type\":\"ALERT\"") != -1) {
    Serial.println("Alert received from mesh network! Forwarding to Firebase...");
    sendToFirebase(from);
  }
}

void newConnectionCallback(uint32_t nodeId) {
  Serial.printf("New mesh node joined: %u\n", nodeId);
}

void setup() {
  Serial.begin(115200);

  // Set verbose debug message types to none initially
  mesh.setDebugMsgTypes( ERROR | STARTUP ); 

  // Initialize painlessMesh bridging mode (AP + STA)
  mesh.init( MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT );
  mesh.onReceive(&receivedCallback);
  mesh.onNewConnection(&newConnectionCallback);
  
  // Manually station the root to the local router
  mesh.stationManual(STATION_SSID, STATION_PASSWORD);
  mesh.setHostname("RootNode");

  // Important configs for the bridge/root
  mesh.setRoot(true);           // Promotes this node to top of the mesh topology
  mesh.setContainsRoot(true);   // Signals other nodes that the root exists here
}

void loop() {
  mesh.update(); // Keep the network alive and check for messages
}
