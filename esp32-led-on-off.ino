#include <WiFi.h>

// WiFi
const char* ssid = "WIFI_NAME";
const char* password = "WIFI_PASSWORD (leave empty when no password)";

// Serwer
WiFiServer server(80);

String header;

// LED (wbudowana dioda ESP32)
const int ledPin = 2;
String ledState = "off";

// Timeout
unsigned long currentTime = millis();
unsigned long previousTime = 0;
const long timeoutTime = 2000;

// ===== TEMPERATURA ESP32 (wbudowany sensor) =====
#ifdef __cplusplus
extern "C" {
  uint8_t temprature_sens_read();
}
#endif

float readTemperature() {
  return (temprature_sens_read() - 32) / 1.8;
}

void setup() {
  Serial.begin(115200);

  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, LOW);

  Serial.println("Laczenie z WiFi...");
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nPolaczono!");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());

  server.begin();
}

void loop() {
  WiFiClient client = server.available();

  if (client) {
    currentTime = millis();
    previousTime = currentTime;

    Serial.println("Nowy klient");
    String currentLine = "";

    while (client.connected() && currentTime - previousTime <= timeoutTime) {
      currentTime = millis();

      if (client.available()) {
        char c = client.read();
        Serial.write(c);
        header += c;

        if (c == '\n') {
          if (currentLine.length() == 0) {

            // ===== HTTP RESPONSE =====
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println("Connection: close");
            client.println();

            // ===== LED CONTROL =====
            if (header.indexOf("GET /on") >= 0) {
              ledState = "on";
              digitalWrite(ledPin, HIGH);
            }
            else if (header.indexOf("GET /off") >= 0) {
              ledState = "off";
              digitalWrite(ledPin, LOW);
            }

            // ===== TEMPERATURA =====
            float temp = readTemperature();

            // ===== HTML =====
            client.println("<!DOCTYPE html><html>");
            client.println("<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");

            client.println("<style>");
            client.println("html { font-family: Helvetica; text-align: center; }");
            client.println(".button { padding: 16px 40px; font-size: 30px; margin: 5px; border: none; color: white; cursor: pointer; }");
            client.println(".on { background-color: #4CAF50; }");
            client.println(".off { background-color: #555555; }");
            client.println("</style>");

            client.println("</head><body>");

            client.println("<h1>ESP32 PANEL</h1>");

            client.println("<p>LED: " + ledState + "</p>");

            if (ledState == "off") {
              client.println("<p><a href=\"/on\"><button class=\"button on\">ON</button></a></p>");
            } else {
              client.println("<p><a href=\"/off\"><button class=\"button off\">OFF</button></a></p>");
            }

            client.println("<hr>");

            client.println("<h2>Temperatura</h2>");
            client.println("<p>" + String(temp) + " °C</p>");

            client.println("</body></html>");
            client.println();

            break;
          }
          else {
            currentLine = "";
          }
        }
        else if (c != '\r') {
          currentLine += c;
        }
      }
    }

    header = "";
    client.stop();
    Serial.println("Klient rozlaczony\n");
  }
}