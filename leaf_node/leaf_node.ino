#include "painlessMesh.h"

#define   MESH_PREFIX     "alertMesh"
#define   MESH_PASSWORD   "alertPassword"
#define   MESH_PORT       5555

#define   BUTTON_PIN      0 // D3 on NodeMCU

Scheduler userScheduler;
painlessMesh  mesh;

unsigned long lastDebounceTime = 0;  
unsigned long debounceDelay = 200;    
int buttonState;             
int lastButtonState = HIGH;   

void setup() {
  Serial.begin(115200);

  // Use internal pullup. Pressing button drops signal to LOW.
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  // Set node type
  mesh.setDebugMsgTypes( ERROR | STARTUP );  

  // Initialize mesh only as AP
  mesh.init( MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT );
  
  Serial.println("Leaf Node initialized. Waiting for button press...");
}

void loop() {
  mesh.update(); // Keep the painlessMesh network running
  
  // Non-blocking button debounce
  int reading = digitalRead(BUTTON_PIN);
  if (reading != lastButtonState) {
    lastDebounceTime = millis();
  }

  if ((millis() - lastDebounceTime) > debounceDelay) {
    if (reading != buttonState) {
      buttonState = reading;
      
      // If the new button state is LOW (pressed)
      if (buttonState == LOW) {
        Serial.println("Button pressed! Broadcasting alert to mesh...");
        
        // Broadcast an alert format
        String msg = "{\"type\":\"ALERT\",\"nodeId\":" + String(mesh.getNodeId()) + "}";
        mesh.sendBroadcast(msg);
      }
    }
  }

  lastButtonState = reading;
}
