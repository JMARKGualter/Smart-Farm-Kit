#include <WiFi.h>
#include <WebServer.h>

#define LED_PIN 27

const char* ssid = "FarmKit_AP";
const char* password = "12345678";

WebServer server(80);
bool ledState = false;

// ================= HTML =================
String htmlPage() {
  String html = "<!DOCTYPE html><html>";
  html += "<head><meta name='viewport' content='width=device-width, initial-scale=1'>";
  html += "<title>Farm Kit LED Control</title>";
  html += "<style>";
  html += "body{font-family:Arial;text-align:center;margin-top:40px;}";
  html += "button{padding:15px 30px;font-size:18px;margin:10px;}";
  html += ".on{background-color:green;color:white;}";
  html += ".off{background-color:red;color:white;}";
  html += "</style></head><body>";

  html += "<h1>Farm Kit LED Control</h1>";
  html += "<p><strong>LED Status: </strong>";
  html += (ledState ? "ON" : "OFF");
  html += "</p>";

  html += "<a href='/on'><button class='on'>TURN ON</button></a>";
  html += "<a href='/off'><button class='off'>TURN OFF</button></a>";

  html += "</body></html>";
  return html;
}

// ================= HANDLERS =================
void handleRoot() {
  server.send(200, "text/html", htmlPage());
}

void handleOn() {
  digitalWrite(LED_PIN, HIGH);
  ledState = true;
  server.send(200, "text/html", htmlPage());
}

void handleOff() {
  digitalWrite(LED_PIN, LOW);
  ledState = false;
  server.send(200, "text/html", htmlPage());
}

// ================= SETUP =================
void setup() {
  Serial.begin(115200);

  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  delay(1000);

  // FORCE AP MODE
  WiFi.mode(WIFI_AP);

  // Start Access Point (visible, channel 6)
  bool result = WiFi.softAP(ssid, password, 6, false, 4);

  if(result) {
    Serial.println("✅ AP Started");
  } else {
    Serial.println("❌ AP Failed");
  }

  Serial.print("AP IP: ");
  Serial.println(WiFi.softAPIP());

  // Routes
  server.on("/", handleRoot);
  server.on("/on", handleOn);
  server.on("/off", handleOff);

  server.begin();
  Serial.println("Web server started");
}

// ================= LOOP =================
void loop() {
  server.handleClient();
}
