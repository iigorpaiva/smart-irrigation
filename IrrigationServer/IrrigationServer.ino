#include <Wire.h>
#include <dummy.h>
#include <WiFi.h>
#include "aWOT.h"
#include "StaticFiles.h"
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <vector>
#include <ArduinoJson.h>

#define WIFI_SSID "brisa-2648486"
#define WIFI_PASSWORD "1hrgpa3r"
#define MODE_BUILTIN 26
#define AOUT_PIN 33

WiFiServer server(80);
Application app;

bool modeOn;

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org");

unsigned long lastConnectionCheckTime = 0;
const unsigned long connectionCheckInterval = 10000;  // Intervalo de verificação em milissegundos (10 segundos)

int programmedDuration;

std::vector<String> scheduleListESP32;

void setup() {
  pinMode(MODE_BUILTIN, OUTPUT);

  Serial.begin(115200);

  connectToWiFi();

  app.get("/mode", &readMode);
  app.get("/moisture", &getMoisture);
  app.get("/duration", &readDuration);
  app.get("/schedule", &readSchedule);
  
  app.put("/mode", &updateMode);

  app.post("/duration", &setDuration);
  app.post("/schedule", &setSchedule);

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
  if (WiFi.status() != WL_CONNECTED) {
    if (currentMillisWifi - lastConnectionCheckTime >= connectionCheckInterval) {
      lastConnectionCheckTime = currentMillisWifi;
      connectToWiFi();
    }
  }

  WiFiClient client = server.available();

  if (client.connected()) {
    app.process(&client);
  }

  // Serial.print("SENSOR: ");
  // Serial.println(readMoistureSensor());
  // delay(600);
}

// ------------------------------------------ Funcoes ------------------------------------

void readDuration(Request &req, Response &res) {
  if (programmedDuration != NULL) {
    res.print(programmedDuration);
  } else {
    programmedDuration = 15;
    res.print(programmedDuration);
  }
}

void setDuration(Request &req, Response &res){
  Serial.print("Duração: ");
  String requestBody = req.readString();
  Serial.println(requestBody);

  programmedDuration = requestBody.toInt();

  res.print(programmedDuration);
}

void readSchedule(Request &req, Response &res) {
  if (scheduleListESP32.empty()) {
    Serial.println("TÁVAZIA!!!");
    res.print("");  // Se a lista estiver vazia, enviar uma string vazia como resposta
    return;
  }

  for (const auto &time : scheduleListESP32) {
    res.print(!time.isEmpty() ? time + "\n" : "");
  }
}

void setSchedule(awot::Request &req, awot::Response &res) {
  // Limpar a lista antes de adicionar novos itens
  scheduleListESP32.clear();

  String requestBody = req.readString();
  Serial.println(requestBody);

  // Parse do JSON usando ArduinoJson
  DynamicJsonDocument doc(1024);  // Tamanho máximo estimado
  deserializeJson(doc, requestBody);

  // Iterar pelos itens no JSON
  for (const auto &item : doc.as<JsonArray>()) {
    String time = item["time"];
    
    // Verificar se o tempo não está vazio antes de adicionar à lista
    if (!time.isEmpty() && time != "null") {
      // Adicionar à lista
      scheduleListESP32.push_back(time);
    }
  }

  for (const auto &time : scheduleListESP32) {
    res.print(time + "\n");
  }
}


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
  int sensorMin = 1760;
  int sensorMax = 4095;

  // Intervalo desejado (porcentagem)
  int targetMin = 100;  // Invertido: agora representa 100% quando molhado
  int targetMax = 0;    // Invertido: agora representa 0% quando seco

  // Converta o valor lido para um intervalo de 0 a 100
  int moisture = map(sensorValue, sensorMin, sensorMax, targetMin, targetMax);

  return moisture;
  // return sensorValue;
}

void readMode(Request &req, Response &res) {
  res.print(modeOn);
}

void updateMode(Request &req, Response &res) {
  String requestBody = req.readString();
  modeOn = (requestBody.toInt() != 0);
  digitalWrite(MODE_BUILTIN, modeOn);

  return readMode(req, res);
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
