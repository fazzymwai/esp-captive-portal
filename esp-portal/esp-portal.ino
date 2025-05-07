#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPClient.h>
#include <DNSServer.h>

const char* local_ssid = "Wifi";
const char* local_password = "passwod";  // Replace with your Wi-Fi password

const char* ap_ssid = "ESP8266-Contact-Form";
const char* ap_password = "";  // Empty password for open network

IPAddress apIP(192, 168, 4, 1);
DNSServer dnsServer;
ESP8266WebServer server(80);

void handleRoot() {
  server.send(200, "text/html", R"rawliteral(
    <!DOCTYPE html>
    <html>
    <head><title>Contact Form</title></head>
    <body>
      <h2>Contact Form</h2>
      <form method="POST" action="/submit">
        <label>Name: <input type="text" name="name"></label><br><br>
        <label>Email: <input type="email" name="email"></label><br><br>
        <label>Message:<br><textarea name="message" rows="5" cols="30"></textarea></label><br><br>
        <input type="submit" value="Send">
      </form>
    </body>
    </html>
  )rawliteral");
}

void handleSubmit() {
  if (server.method() == HTTP_POST) {
    String name = server.arg("name");
    String email = server.arg("email");
    String message = server.arg("message");

    Serial.println("Form Submitted:");
    Serial.println("Name: " + name);
    Serial.println("Email: " + email);
    Serial.println("Message: " + message);

    // Only try to send to Flask server if connected to local WiFi
    if (WiFi.status() == WL_CONNECTED) {
      // Prepare JSON payload
      String jsonData = "{\"name\":\"" + name + "\",\"email\":\"" + email + "\",\"message\":\"" + message + "\"}";

      // Send data to Flask server
      HTTPClient http;
      WiFiClient client;
      http.begin(client, "http://192.168.server IP:5000/submit");
      http.addHeader("Content-Type", "application/json");

      int httpResponseCode = http.POST(jsonData);

      Serial.print("Flask Response Code: ");
      Serial.println(httpResponseCode);
      String response = http.getString();
      Serial.println("Flask Response Body: " + response);

      http.end();
      server.send(200, "text/html", "<h3>Thank you! Your message was sent to the server.</h3>");
    } else {
      server.send(200, "text/html", "<h3>Thank you! Your message was saved locally (not sent to server).</h3>");
    }
  } else {
    server.send(405, "text/plain", "Method Not Allowed");
  }
}

void handleNotFound() {
  // Redirect all unknown requests to the root page for captive portal effect
  server.sendHeader("Location", "http://192.168.4.1", true);
  server.send(302, "text/plain", "");
}

void setup() {
  Serial.begin(115200);
  
  // Start in both STA (station) and AP (access point) mode
  WiFi.mode(WIFI_AP_STA);
  
  // Connect to local WiFi
  WiFi.begin(local_ssid, local_password);
  Serial.print("Connecting to local Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\n✅ Connected to local WiFi!");
  Serial.print("Local IP Address: ");
  Serial.println(WiFi.localIP());

  // Set up access point
  WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
  WiFi.softAP(ap_ssid, ap_password);
  Serial.print("Access Point IP address: ");
  Serial.println(WiFi.softAPIP());

  // DNS server for captive portal
  dnsServer.start(53, "*", apIP);

  // Set up web server
  server.on("/", HTTP_GET, handleRoot);
  server.on("/submit", HTTP_POST, handleSubmit);
  server.onNotFound(handleNotFound);  // Redirect all other requests
  server.begin();
  Serial.println("📡 HTTP server started");
}

void loop() {
  dnsServer.processNextRequest();
  server.handleClient();
}