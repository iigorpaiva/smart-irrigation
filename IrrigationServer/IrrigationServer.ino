#include <Wire.h>
#include <dummy.h>
#include <WiFi.h>
#include "aWOT.h"
#include "StaticFiles.h"
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <vector>
#include <ArduinoJson.h>
#include <algorithm>
#include <map>
#include <ESPmDNS.h>

#define AVERAGE_CALCULATION_INTERVAL 60000  // Intervalo de cálculo da média a cada 1 minuto
#define PRINT_MAP_INTERVAL 5000  // Intervalo de impressão do mapa a cada 5 segundos

#define MAX_MAP_SIZE 12

#define WIFI_SSID "brisa-2648486-EXT"
#define WIFI_PASSWORD "1hrgpa3r"
#define MODE_BUILTIN 26
#define AOUT_PIN 33

WiFiServer server(80);
Application app;

bool modeOn;
bool activatedBySchedule = false;

// Define NTP Client to get time
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);

unsigned long lastConnectionCheckTime = 0;
const unsigned long connectionCheckInterval = 10000;  // Intervalo de verificação em milissegundos (10 segundos)

const unsigned long updateInterval = 1000;  // Atualizar a cada 1 minuto
unsigned long lastUpdateTime = 0;

int programmedDuration = 2;

unsigned long modeActivationStartTime = 0;
unsigned long modeActivationDuration = 0;

unsigned long lastPrintMapTime = 0;

std::vector<String> scheduleListESP32;

std::map<String, float> sensorHourlyAverageMap;

const char* host = "autoirrigacao.local";

void setup() {
  pinMode(MODE_BUILTIN, OUTPUT);

  Serial.begin(115200);

  connectToWiFi();

  if(!MDNS.begin(host)){
    Serial.println("Erro ao configurar DNS!");
    while(1){
      delay(1000);
    }
  }

  app.get("/mode", &readMode);
  app.get("/moisture", &getMoisture);
  app.get("/duration", &getDuration);
  app.get("/schedule", &readSchedule);
  app.get("/chartData", &getChartData);
  
  app.put("/mode", &updateMode);

  app.post("/duration", &setDuration);
  app.post("/schedule", &setSchedule);

  app.use(staticFiles());

  server.begin();
  
  if (MDNS.begin("autoirrigacao")) {
    MDNS.addService("http", "tcp", 80);  // Adiciona o tipo de serviço HTTP
    Serial.println("mDNS responder iniciado");
  } else {
    Serial.println("Erro ao iniciar o mDNS responder");
  }

  Serial.println("Informações sobre a memória Flash:");
  Serial.printf("Tamanho total: %.2f MB\n", ESP.getFlashChipSize() / (1024.0 * 1024.0));
  Serial.printf("Tamanho livre: %.2f MB\n", ESP.getFreeSketchSpace() / (1024.0 * 1024.0));

  // Inicialização do cliente NTP
  timeClient.begin();
  timeClient.setTimeOffset(-3 * 3600);
}

void loop() {
  unsigned long currentMillis = millis();

  if (currentMillis - lastUpdateTime >= updateInterval) {
    timeClient.forceUpdate();
    lastUpdateTime = currentMillis;
  }

  checkScheduledMode();

  // Verificar e reconectar se necessário a cada 10 segundos
  if (WiFi.status() != WL_CONNECTED) {
    if (currentMillis - lastConnectionCheckTime >= connectionCheckInterval) {
      lastConnectionCheckTime = currentMillis;
      connectToWiFi();
    }
  }

  // Verificar se há um cliente disponível
  if (server.hasClient()) {
    // Obter o cliente apenas se estiver disponível
    WiFiClient client = server.available();
    app.process(&client);
  }

  // Serial.print("SENSOR: ");
  // Serial.println(readMoistureSensor());
  // delay(600);

  // printCurrentTime();

  calculateAverageSensorHour();

   // Chamar a função para imprimir o mapa a cada 5 segundos
  // if (currentMillisWifi - lastPrintMapTime >= PRINT_MAP_INTERVAL) {
  //   lastPrintMapTime = currentMillisWifi;
  //   printSensorHourlyAverageMap();
  // }
}

// ------------------------------------------ Funcoes ------------------------------------

void printSensorHourlyAverageMap() {
  Serial.println("Sensor Hourly Average Map:");

  for (const auto &entry : sensorHourlyAverageMap) {
    Serial.print("Hour: ");
    Serial.print(entry.first);
    Serial.print(", Average Moisture: ");
    Serial.println(entry.second);
  }

  Serial.println();
}

void calculateAverageSensorHour() {
  static unsigned long lastAverageCalculationTime = 0;
  static float sumMoisture = 0;
  static int numReadings = 0;
  String currentHour;

  unsigned long currentMillis = millis();

  // Verificar se é hora de calcular a média
  if (currentMillis - lastAverageCalculationTime >= AVERAGE_CALCULATION_INTERVAL) {
    // Calcular a média e armazenar no mapa
    if (numReadings > 0) {

      // Remover os minutos da hora para salvar apenas a hora no mapa
      currentHour = convertToHHMM0(timeClient.getFormattedTime());
      Serial.print("ANTES: ");
      Serial.println(currentHour);

      currentHour.remove(3, 2); // Remover minutos
      currentHour.remove(6, 5); // Remover segundos
      currentHour += ":00:00.000Z";
      Serial.print("DEPOIS: ");
      Serial.println(currentHour);

      // Serial.print("current hour: ");
      // Serial.println(currentHour);

      // Verificar se já existe uma entrada para a hora atual no mapa
      auto it = sensorHourlyAverageMap.find(currentHour);
      if (it != sensorHourlyAverageMap.end()) {
        // Se existir, atualizar a média
        it->second = sumMoisture / numReadings;
        Serial.print("média atualizada: ");
        Serial.println(sensorHourlyAverageMap[currentHour]);
      } else {
        // Se não existir, adicionar uma nova entrada no mapa
        sensorHourlyAverageMap[currentHour] = sumMoisture / numReadings;
      }
    }

    lastAverageCalculationTime = currentMillis;

    // Verificar e ajustar o tamanho do mapa para o máximo permitido
    while (sensorHourlyAverageMap.size() > MAX_MAP_SIZE) {
      sensorHourlyAverageMap.erase(sensorHourlyAverageMap.begin());
    }
  }

  // Fazer a leitura do sensor a cada minuto e adicionar ao cálculo da média
  if ((currentMillis - lastAverageCalculationTime) < AVERAGE_CALCULATION_INTERVAL) {
    int moistureReading = readMoistureSensor();
    sumMoisture += moistureReading;
    numReadings++;
  }
}

void checkScheduledMode() {
  String currentTime = convertToHHMM(timeClient.getFormattedTime());

  // Verificar se o modo está desativado manualmente
  if (!modeOn) {
    if (std::find(scheduleListESP32.begin(), scheduleListESP32.end(), currentTime) != scheduleListESP32.end()) {
      // O tempo atual está na lista de horários programados
      modeOn = true;
      digitalWrite(MODE_BUILTIN, modeOn);
      Serial.println("Modo ativado conforme programação de horário.");

      // Configurar o tempo de início e duração
      modeActivationStartTime = millis();
      modeActivationDuration = programmedDuration * 60 * 1000; // Convertendo minutos para milissegundos

      activatedBySchedule = true;
      
    }
  } 

  // Verificar se o modo está ativado e se o tempo de duração expirou
  if (activatedBySchedule && ((millis() - modeActivationStartTime) >= modeActivationDuration)) {
    modeOn = false;
    activatedBySchedule = false;
    digitalWrite(MODE_BUILTIN, modeOn);
    Serial.println("Modo desativado após o tempo programado.");
  }
  
}

String convertToHHMM(String formattedTime) {
  int separatorIndex = formattedTime.indexOf(':');
  if (separatorIndex != -1) {
    return formattedTime.substring(0, separatorIndex + 3);
  }
  return formattedTime;
}

String convertToHHMM0(String formattedTime) {
  int separatorIndex = formattedTime.indexOf(':');
  if (separatorIndex != -1) {
    return formattedTime.substring(0, separatorIndex);
  }
  return formattedTime;
}

void printCurrentTime() {
  Serial.print("Hora atual: ");
  Serial.print(timeClient.getFormattedTime());
  Serial.println();
}

void getDuration(Request &req, Response &res) {
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


void setSchedule(Request &req, Response &res) {
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

  // Ordenar a lista
  std::sort(scheduleListESP32.begin(), scheduleListESP32.end());

  for (const auto &time : scheduleListESP32) {
    res.print(time + "\n");
  }
}

void getChartData(Request &req, Response &res) {
  // Criar um objeto DynamicJsonDocument para armazenar os dados do gráfico
  DynamicJsonDocument chartData(1024);

  // Preencher o objeto com dados reais do seu mapa (sensorHourlyAverageMap)
  for (const auto &entry : sensorHourlyAverageMap) {
    // Converta o valor para um número inteiro
    int intValue = static_cast<int>(entry.second);
    chartData[entry.first] = intValue;
  }

  // Serializar o objeto JSON para uma string
  String jsonString;
  serializeJson(chartData, jsonString);

  // Configurar o cabeçalho da resposta para indicar que é JSON
  res.set("Content-Type", "application/json");

  // Enviar os dados como resposta JSON
  Serial.print("o que tá indo pra lá: ");
  Serial.println(jsonString);

  res.print(jsonString);
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
  int sensorMin = 3120;
  int sensorMax = 4095;

  // Intervalo desejado (porcentagem)
  int targetMin = 100;  // Invertido: agora representa 100% quando molhado
  int targetMax = 0;    // Invertido: agora representa 0% quando seco

  // Converta o valor lido para um intervalo de 0 a 100
  int moisture = map(sensorValue, sensorMin, sensorMax, targetMin, targetMax);

  // return moisture;
  return sensorValue;
}

void readMode(Request &req, Response &res) {
  res.print(modeOn);
}

void updateMode(Request &req, Response &res) {
  String requestBody = req.readString();
  bool newMode = (requestBody.toInt() != 0);
  activatedBySchedule = false;

  // Atualizar o estado do modo
  modeOn = newMode;
  digitalWrite(MODE_BUILTIN, modeOn);

  Serial.println("Modo atualizado via requisição HTTP.");

  readMode(req, res);  // Atualizar a resposta com o estado atual do modo
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
