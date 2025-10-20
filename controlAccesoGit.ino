#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <SPI.h>
#include <MFRC522.h>
#include <Servo.h>

// --- Configuración de Red ---
const char* ssid = "---------";
const char* password = "---------";

// --- Configuración proto ---
#define SS_PIN    D4
#define RST_PIN   D3
#define SERVO_PIN D1

// --- Objetos ---
MFRC522 mfrc522(SS_PIN, RST_PIN);
Servo miServo;
ESP8266WebServer server(80);

// --- Lista de Tarjetas ---
const int numeroDeTarjetas = 1; 
String uidsAutorizados[numeroDeTarjetas] = {
  "AB CD EF GH"  
 
};

String logDeAccesos = "";
// --- CONTADOR ---
int accesosConcedidos = 0;
int accesosDenegados = 0;


void setup() {
  Serial.begin(9600);
  
  SPI.begin();
  mfrc522.PCD_Init();
  miServo.attach(SERVO_PIN);
  miServo.write(0);
  
  // Conexión WiFi
  WiFi.begin(ssid, password);
  Serial.print("Conectando a ");
  Serial.println(ssid);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi conectado!");
  Serial.print("Dirección IP: ");
  Serial.println(WiFi.localIP());

  // servidor
  server.on("/", handleRoot);
  server.begin();
  Serial.println("Servidor HTTP iniciado.");
  logDeAccesos = "<p>Sistema iniciado.</p>";
}

void loop() {
  server.handleClient();

  if (!mfrc522.PICC_IsNewCardPresent() || !mfrc522.PICC_ReadCardSerial()) {
    return;
  }

  String uidLeido = "";
  for (byte i = 0; i < mfrc522.uid.size; i++) {
    uidLeido.concat(String(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " "));
    uidLeido.concat(String(mfrc522.uid.uidByte[i], HEX));
  }
  uidLeido.toUpperCase();
  uidLeido.trim();

  Serial.print("Tarjeta detectada: ");
  Serial.println(uidLeido);

  bool accesoPermitido = false;
  for (int i = 0; i < numeroDeTarjetas; i++) {
    if (uidLeido == uidsAutorizados[i]) {
      accesoPermitido = true;
      break;
    }
  }

  String logEntry;
  
  if (accesoPermitido) {
    Serial.println("Acceso Concedido!");
    miServo.write(180);
    logEntry = "<p class='concedido'><b>ACCESO CONCEDIDO</b> - UID: " + uidLeido + "</p>";
    accesosConcedidos++; 
    delay(2000);
    miServo.write(0);
  } else {
    Serial.println("Acceso Denegado.");
    logEntry = "<p class='denegado'><b>ACCESO DENEGADO</b> - UID: " + uidLeido + "</p>";
    accesosDenegados++; 
  }
  
  logDeAccesos = logEntry + logDeAccesos;
  delay(1000);
}


// --- página HTML ---
void handleRoot() {
  String page = "<!DOCTYPE html><html lang='es'><head>";
  page += "<meta charset='UTF-8'>";
  page += "<meta name='viewport' content='width=device-width, initial-scale=1.0'>";
  page += "<title>Monitor de Acceso</title>";
  page += "<meta http-equiv='refresh' content='5'>";
  
  page += "<style>";
  page += "body { font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif; background-color: #2c2a4a; color: #f0f0f8; margin: 0; padding: 20px; display: flex; flex-direction: column; align-items: center; min-height: 100vh; }";
  page += "h1 { color: #9e8dff; font-size: 2.5em; text-shadow: 2px 2px 4px rgba(0,0,0,0.2); }";

  page += ".counters { display: flex; justify-content: space-around; width: 90%; max-width: 600px; margin-bottom: 20px; background-color: #4f4a7d; padding: 15px; border-radius: 12px; box-shadow: 0 10px 20px rgba(0,0,0,0.3); }";
  page += ".counter { text-align: center; }";
  page += ".counter h2 { margin: 0; font-size: 1.2em; color: #aaa8c2; }";
  page += ".counter .count { font-size: 2.5em; font-weight: bold; }";
  page += ".count.concedido { color: #50fa7b; }";
  page += ".count.denegado { color: #ff5555; }";

  page += ".log-container { background-color: #4f4a7d; border-radius: 12px; padding: 25px; width: 90%; max-width: 600px; box-shadow: 0 10px 20px rgba(0,0,0,0.3); }";
  page += ".log-entry p { margin: 8px 0; padding: 10px; border-left: 4px solid; border-radius: 4px; }";
  page += ".log-entry .concedido { border-color: #50fa7b; background-color: rgba(80, 250, 123, 0.1); color: #d4ffea; }";
  page += ".log-entry .denegado { border-color: #ff5555; background-color: rgba(255, 85, 85, 0.1); color: #ffd6d6; }";
  page += "footer { margin-top: auto; padding-top: 20px; text-align: center; color: #aaa8c2; font-size: 0.9em; }";
  page += "footer .brand { font-weight: bold; }";
  page += "</style>";
  
  page += "</head><body>";
  page += "<h1>Monitor de acceso</h1>";
  
  page += "<div class='counters'>";
  page += "<div class='counter'><h2>Concedidos</h2><span class='count concedido'>" + String(accesosConcedidos) + "</span></div>";
  page += "<div class='counter'><h2>Denegados</h2><span class='count denegado'>" + String(accesosDenegados) + "</span></div>";
  page += "</div>";

  page += "<div class='log-container'><h2>Registros</h2>";
  page += "<div class='log-entry'>" + logDeAccesos + "</div>";
  page += "</div>";

  page += "<footer>";
  page += "Matias Cerna - Luciano Aliaga<br>";
  page += "<span class='brand'>@TuplaTech</span>";
  page += "</footer>";
  
  page += "</body></html>";
  
  server.send(200, "text/html", page);
}