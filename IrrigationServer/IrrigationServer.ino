#include <Wire.h>
#include <dummy.h>
#include <WiFi.h>
#include "aWOT.h"
#include "StaticFiles.h"
#include <NTPClient.h>
#include <WiFiUdp.h>

#define WIFI_SSID "brisa-2648486-EXT"
#define WIFI_PASSWORD "1hrgpa3r"
#define LED_BUILTIN 26
#define AOUT_PIN 33

WiFiServer server(80);
Application app;

bool ledOn;

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org");

unsigned long lastConnectionCheckTime = 0;
const unsigned long connectionCheckInterval = 10000;  // Intervalo de verificação em milissegundos (10 segundos)

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  
  Serial.begin(115200);

  connectToWiFi();

  app.get("/led", &readLed);
  app.put("/led", &updateLed);
  app.get("/moisture", &getMoisture);  // Nova rota para obter valores do sensor de umidade

  app.use(staticFiles());

  server.begin();

  Serial.println("Informações sobre a memória Flash:");
  Serial.printf("Tamanho total: %.2f MB\n", ESP.getFlashChipSize() / (1024.0 * 1024.0));
  Serial.printf("Tamanho livre: %.2f MB\n", ESP.getFreeSketchSpace() / (1024.0 * 1024.0));

  // Inicialização do cliente NTP
  timeClient.begin();
}

void loop() {

  unsigned long currentMillisWifi = millis();

  // Verificar e reconectar se necessário a cada 10 segundos
  if (currentMillisWifi - lastConnectionCheckTime >= connectionCheckInterval) {
    lastConnectionCheckTime = currentMillisWifi;

    if (WiFi.status() != WL_CONNECTED) {
      Serial.println("Conexão perdida. Tentando reconectar...");
      connectToWiFi();
    }
  }

  WiFiClient client = server.available();

  if (client.connected()) {
    app.process(&client);
  }

    // Imprime o valor do sensor a cada segundo
  // unsigned long currentMillis = millis();
  // static unsigned long previousMillis = 0;
  // if (currentMillis - previousMillis >= 1000) {
  //   previousMillis = currentMillis;

  //   int sensorMoisture = readMoistureSensor();
  //   Serial.println(sensorMoisture);

  // }

    // int sensorMoisture = readMoistureSensor();
    // Serial.println(sensorMoisture);
    // delay(100);
}

// ------------------------------------------ Funcoes ------------------------------------


// Rota para obter valores do sensor de umidade
void getMoisture(Request &req, Response &res) {
  int sensorMoisture = readMoistureSensor();
  res.print(sensorMoisture);
}

// // Função para ler valores do sensor de umidade
int readMoistureSensor() {
  // Leitura analógica do sensor de umidade
  int sensorValue = analogRead(AOUT_PIN);

  // Intervalo original do sensor
  int sensorMin = 1126;
  int sensorMax = 3055;

  // Intervalo desejado (porcentagem)
  int targetMin = 100;  // Invertido: agora representa 100% quando molhado
  int targetMax = 0;    // Invertido: agora representa 0% quando seco

  // Converta o valor lido para um intervalo de 0 a 100
  int moisture = map(sensorValue, sensorMin, sensorMax, targetMin, targetMax);

  return moisture;
}


void readLed(Request &req, Response &res) {
  res.print(ledOn);
}

void updateLed(Request &req, Response &res) {
  ledOn = (req.read() != '0');
  digitalWrite(LED_BUILTIN, ledOn);
  return readLed(req, res);
}

void connectToWiFi() {
  Serial.println("Conectando ao WiFi...");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nConectado com sucesso ao WiFi");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\nFalha na conexão ao WiFi. Verifique suas credenciais.");
  }
}

