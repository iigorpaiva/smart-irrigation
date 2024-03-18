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
// #include <WiFiManager.h> 

#define AVERAGE_CALCULATION_INTERVAL 60000  // Intervalo de cálculo da média a cada 1 minuto
#define CHECKING_SCHEDULED_ACTIVATION_INTERVAL 60000  // Intervalo de checagem para ativação dos horários programados a cada 1 minuto
#define PRINT_MAP_INTERVAL 5000  // Intervalo de impressão do mapa a cada 5 segundos

#define MAX_MAP_SIZE 12

#define WIFI_SSID "brisa-2648486"
#define WIFI_PASSWORD "1hrgpa3r"
#define MODE_BUILTIN 26
#define AOUT_PIN 33

WiFiServer server(80);
Application app;

bool modeOn;
bool sensorMode;
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

  // WiFiManager wifiManager;

  // wifiManager.resetSettings();

  // wifiManager.setConfigPortalTimeout(60);

  // if (!wifiManager.autoConnect("AutoIrrigacaoAP")) {
  //   Serial.println("Falha na conexão e tempo limite atingido, reiniciando...");
  //   ESP.restart();
  // }

  connectToWiFi();

  // if(!MDNS.begin(host)){
  //   Serial.println("Erro ao configurar DNS!");
  //   while(1){
  //     delay(1000);
  //   }
  // }

  app.get("/mode", &readMode);
  app.get("/sensorMode", &readSensorMode);
  app.get("/moisture", &getMoisture);
  app.get("/duration", &getDuration);
  app.get("/schedule", &readSchedule);
  app.get("/chartData", &getChartData);
  
  app.put("/mode", &updateMode);
  app.put("/sensorMode", &updateSensorMode);

  app.post("/duration", &setDuration);
  app.post("/schedule", &setSchedule);

  app.use(staticFiles());

  server.begin();
  
  if (MDNS.begin("autoirrigacao")) {
    MDNS.addService("http", "tcp", 80);  // Adiciona o tipo de serviço HTTP
    // Serial.println("mDNS responder iniciado");
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

  // //  Chamar a função para imprimir o mapa a cada 5 segundos
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
  String receivedHour;
  String currentHour;
  String currentHourWithMinutes;
  String lastHour;

  unsigned long currentMillis = millis();

  // Verificar se é hora de calcular a média
  if (currentMillis - lastAverageCalculationTime >= AVERAGE_CALCULATION_INTERVAL) {

    receivedHour = timeClient.getFormattedTime();
    currentHourWithMinutes = convertToHHMM(receivedHour);

    // Calcular a média e armazenar no mapa
    if (numReadings > 0) {

      // Verificar se a hora recebida faz sentido (por exemplo, não é 03:00 quando não deveria ser)
      if (isValidTime(receivedHour)) {

        // Remover os minutos da hora para salvar apenas a hora no mapa
        currentHour = convertToHHMM0(receivedHour);

        currentHour.remove(3, 2); // Remover minutos
        currentHour.remove(6, 5); // Remover segundos
        currentHour += ":00:00.000Z";

        // Verificar se já existe uma entrada para a hora atual no mapa
        auto it = sensorHourlyAverageMap.find(currentHour);
        if (it != sensorHourlyAverageMap.end()) {
          // Se existir, atualizar a média
          it->second = sumMoisture / numReadings;
          Serial.print("Média atualizada: ");
          Serial.println(sensorHourlyAverageMap[currentHour]);
          Serial.print("");
        } else {
          // Se não existir, adicionar uma nova entrada no mapa
          sensorHourlyAverageMap[currentHour] = sumMoisture / numReadings;
        }
      }
    }

    lastAverageCalculationTime = currentMillis;

    // Verificar e ajustar o tamanho do mapa para o máximo permitido
    // while (sensorHourlyAverageMap.size() > MAX_MAP_SIZE) {
    //   sensorHourlyAverageMap.erase(sensorHourlyAverageMap.begin());
    // }
  }

  // Verificar se é meia-noite e limpar a lista se necessário
  if (currentHourWithMinutes == "00:00" || currentHourWithMinutes == "00:01") {
    sensorHourlyAverageMap.clear();
    Serial.println("Lista reiniciada pois é meia-noite.");
  }

  // Fazer a leitura do sensor a cada minuto e adicionar ao cálculo da média
  if ((currentMillis - lastAverageCalculationTime) < AVERAGE_CALCULATION_INTERVAL) {
    int moistureReading = readMoistureSensor();
    sumMoisture += moistureReading;
    numReadings++;
  }
}

// Essa função checa se a hora é válida
// Em alguns momentos aleatórios, a hora vinha errada sempre marcando 3 horas da manhã por alguns minutos 
// Logo depois essa hora voltava ao normal, seguindo a sequência dos horários
// A solução é checar se a hora que está vindo é igual a última hora registrada da lista ou se é uma hora que vem depois da última 
bool isValidTime(String receivedHour) {
  // Extrair as horas e minutos do formato HH:MM:SS
  int receivedHours = receivedHour.substring(0, 2).toInt();

  // Verificar se o mapa de médias horárias do sensor está vazio
  if (sensorHourlyAverageMap.empty()) {
    Serial.println("Validado pelo mapa de médias horárias vazio");
    // Se o mapa estiver vazio, a hora recebida é considerada válida
    return true;
  }

  // Obter a última hora no mapa (última entrada)
  auto lastEntry = sensorHourlyAverageMap.rbegin();
  String lastHour = lastEntry->first;

  // Extrair as horas e minutos da última hora no mapa
  int lastHourHours = lastHour.substring(0, 2).toInt();

  // Verificar se a hora recebida é igual à última hora ou se é a próxima hora após a última no mapa
  if ((receivedHours == lastHourHours) || receivedHours == (lastHourHours + 1)) {
    // Se a hora recebida for igual à última hora ou a próxima hora, considera como válida
    Serial.print("A última hora no mapa é: ");
    Serial.println(lastHourHours);
    Serial.print("A hora atual é: ");
    Serial.println(receivedHours);
    return true;
  }
  Serial.println("False retornado!");
  // Se a hora recebida não atender às condições, considera como inválida
  return false;
}

void checkScheduledMode() {
  static unsigned long lastScheduledCheckTime = 0;
  static unsigned long lastMinute = 0;  // Variável para armazenar o minuto anterior

  unsigned long currentMillis = millis();
  unsigned long currentMinute = timeClient.getMinutes();

  // Verificar se houve uma mudança no minuto
  if (currentMinute != lastMinute) {
    lastMinute = currentMinute;

    // Realizar a lógica de verificação apenas na virada dos minutos
    String currentTime = convertToHHMM(timeClient.getFormattedTime());

    // Verificar se sensorMode é true
    if (sensorMode) {
      // Verificar se o modo está desativado manualmente
      if (!modeOn) {
        // Verificar se o valor retornado pelo readMoistureSensor() é abaixo de 60%
        if (readMoistureSensor() < 60) {
          if (std::find(scheduleListESP32.begin(), scheduleListESP32.end(), currentTime) != scheduleListESP32.end()) {
            // O tempo atual está na lista de horários programados
            modeOn = true;
            digitalWrite(MODE_BUILTIN, modeOn);
            Serial.println("Modo ativado conforme programação de horário e umidade abaixo de 60%.");

            // Configurar o tempo de início e duração
            modeActivationStartTime = currentMillis;
            modeActivationDuration = programmedDuration * 60 * 1000; // Convertendo minutos para milissegundos

            activatedBySchedule = true;
          }
        }
      }
    }
  }

  // Verificar se o modo está ativado e se o tempo de duração expirou
  if (activatedBySchedule && ((currentMillis - modeActivationStartTime) >= modeActivationDuration)) {
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
    // Se a lista de horários estiver vazia, retorne uma resposta vazia
    res.end();
    return;
  }

  // Caso contrário, retorne os horários programados normalmente
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

  // Verificar se a lista está vazia
  if (scheduleListESP32.empty()) {
    // Se a lista de horários estiver vazia, retorne uma resposta vazia
    res.end();
    return;
  }

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
  // Serial.print("o que tá indo pra lá: ");
  Serial.println(jsonString);

  if (jsonString.length() == 0 || jsonString.isEmpty() ) {
    // Se jsonString está vazio, retorne uma resposta vazia
    Serial.println("ENTROU?");
    res.end();
    return;
  }

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
  int sensorMin = 1017; // Totalmente úmido
  int sensorMax = 2889; // Totalmente seco

  // Converta o valor lido para um intervalo de 0 a 100
  int moisture = map(sensorValue, sensorMin, sensorMax, 100, 0);

  // Garanta que os valores não ultrapassem os limites
  moisture = constrain(moisture, 0, 100);

  return moisture;
}

void readMode(Request &req, Response &res) {
  res.print(modeOn);
}

void readSensorMode(Request &req, Response &res) {
  res.print(sensorMode);
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

void updateSensorMode(Request &req, Response &res) {
  String requestBody = req.readString();
  bool newMode = (requestBody.toInt() != 0);

  // Atualizar o estado do modo
  sensorMode = newMode;
  // Serial.println("Modo do sensor atualizado via requisição HTTP.");
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
    WiFi.disconnect(); // Desconectar para liberar recursos
  }
}
