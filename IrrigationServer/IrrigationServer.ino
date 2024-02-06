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

WiFiServer server(80);
Application app;

bool ledOn;

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org");

// Função para gerar valores de umidade aleatórios (simulando o sensor)
int generateRandomMoisture() {
  return random(0, 101);  // Gera valores entre 0 e 100
}

void readLed(Request &req, Response &res) {
  res.print(ledOn);
}

void updateLed(Request &req, Response &res) {
  ledOn = (req.read() != '0');
  digitalWrite(LED_BUILTIN, ledOn);
  return readLed(req, res);
}

void getTime(Request &req, Response &res) {
  timeClient.update();
  res.print(timeClient.getFormattedTime());
}

void getWeather(Request &req, Response &res) {
  // Lógica para obter a previsão do tempo de algum lugar
  String weatherData = "Sua Previsão do Tempo Aqui";
  res.print(weatherData);
}

// Rota para obter valores simulados de umidade
void getMoisture(Request &req, Response &res) {
  int simulatedMoisture = generateRandomMoisture();
  res.print(simulatedMoisture);
}

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  
  Serial.begin(115200);

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println(WiFi.localIP());

  app.get("/led", &readLed);
  app.put("/led", &updateLed);
  app.get("/time", &getTime);          // Nova rota para obter a hora
  app.get("/weather", &getWeather);    // Nova rota para obter a previsão do tempo
  app.get("/moisture", &getMoisture);  // Nova rota para obter valores simulados de umidade

  app.use(staticFiles());

  server.begin();

  Serial.println("Informações sobre a memória Flash:");
  Serial.printf("Tamanho total: %.2f MB\n", ESP.getFlashChipSize() / (1024.0 * 1024.0));
  Serial.printf("Tamanho livre: %.2f MB\n", ESP.getFreeSketchSpace() / (1024.0 * 1024.0));

  // Inicialização do cliente NTP
  timeClient.begin();
}

void loop() {
  WiFiClient client = server.available();

  if (client.connected()) {
    app.process(&client);
  }
}
