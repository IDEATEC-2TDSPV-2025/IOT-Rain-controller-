#include <WiFi.h>
#include <ThingerESP32.h>

// === CONFIGURAÇÕES THINGER.IO ===
#define USERNAME "JPAmorimBV"          
#define DEVICE_ID "rain_controller"           
#define DEVICE_CREDENTIAL "Device123" 

// === CONFIGURAÇÕES WiFi PARA WOKWI ===
#define SSID "Wokwi-GUEST"
#define SSID_PASSWORD "" 

// === DEFINIÇÃO DOS PINOS ===
#define TRIG_PIN 5
#define ECHO_PIN 18
#define RAIN_SENSOR_PIN 34
#define LED_RED_PIN 25
#define LED_GREEN_PIN 26
#define LED_BLUE_PIN 27
#define BUZZER_PIN 13

// === CONFIGURAÇÕES DO SISTEMA ===
#define WATER_TANK_HEIGHT 100
#define ALERT_THRESHOLD 70
#define EMERGENCY_THRESHOLD 90
#define MEASUREMENT_INTERVAL 3000 

// === INSTÂNCIA THINGER.IO ===
ThingerESP32 thing(USERNAME, DEVICE_ID, DEVICE_CREDENTIAL);

// === VARIÁVEIS GLOBAIS ===
float waterLevel = 0.0;
int rainIntensity = 0;
String systemStatus = "NORMAL";
unsigned long lastMeasurement = 0;
bool buzzerActive = false;
bool emergencyAlerted = false;

// === VARIÁVEIS DE SIMULAÇÃO ===
int simulationMode = 0;
unsigned long simulationTimer = 0;
bool increasingWater = true;
float simulatedDistance = 80.0; 

void setup() {
  Serial.begin(115200);
  Serial.println("=== SISTEMA DE MONITORAMENTO DE ENCHENTES ===");
  
  // === CONFIGURAÇÃO DOS PINOS ===
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(RAIN_SENSOR_PIN, INPUT);
  pinMode(LED_RED_PIN, OUTPUT);
  pinMode(LED_GREEN_PIN, OUTPUT);
  pinMode(LED_BLUE_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  
  // === CONEXÃO WiFi ===
  Serial.println("Conectando ao WiFi Wokiwi...");
  thing.add_wifi(SSID, SSID_PASSWORD);
  
  // Aguarda conexão WiFi
  int wifiTimeout = 0;
  while (WiFi.status() != WL_CONNECTED && wifiTimeout < 20) {
    delay(1000);
    Serial.print(".");
    wifiTimeout++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println();
    Serial.println("✅ WiFi conectado!");
    Serial.print("IP: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println();
  }
  
  // === CONFIGURAÇÃO DOS RECURSOS THINGER.IO ===
  
  // DADOS DE SAÍDA (OUTPUT) - Para dashboards e monitoramento
  thing["water_level"] >> [](pson& out) {
    out = waterLevel;
  };
  
  thing["rain_intensity"] >> [](pson& out) {
    out = rainIntensity;
  };
  
  thing["system_status"] >> [](pson& out) {
    out = systemStatus;
  };
  
  //DADOS COMPLETOS PARA DASHBOARD
  thing["sensor_data"] >> [](pson& out) {
    out["water_level"] = waterLevel;
    out["rain_intensity"] = rainIntensity;
    out["status"] = systemStatus;
    out["timestamp"] = millis();
    out["distance_cm"] = simulatedDistance;
    out["location"]["lat"] = -23.5505; 
    out["location"]["lon"] = -46.6333;
    out["tank_height"] = WATER_TANK_HEIGHT;
    out["alert_threshold"] = ALERT_THRESHOLD;
    out["emergency_threshold"] = EMERGENCY_THRESHOLD;
    out["simulation_mode"] = simulationMode;
  };
  
  //dashboard
  thing["led_control"] << [](pson& in) {
    if(!in["red"].is_empty()) {
      digitalWrite(LED_RED_PIN, in["red"] ? HIGH : LOW);
      Serial.println("💡 LED Vermelho: " + String(in["red"] ? "ON" : "OFF"));
    }
    if(!in["green"].is_empty()) {
      digitalWrite(LED_GREEN_PIN, in["green"] ? HIGH : LOW);
      Serial.println("💡 LED Verde: " + String(in["green"] ? "ON" : "OFF"));
    }
    if(!in["blue"].is_empty()) {
      digitalWrite(LED_BLUE_PIN, in["blue"] ? HIGH : LOW);
      Serial.println("💡 LED Azul: " + String(in["blue"] ? "ON" : "OFF"));
    }
  };
  
  thing["buzzer_control"] << [](pson& in) {
    buzzerActive = in;
    digitalWrite(BUZZER_PIN, buzzerActive ? HIGH : LOW);
    Serial.println("🔊 Buzzer controlado remotamente: " + String(buzzerActive ? "ON" : "OFF"));
  };
  
  //CONTROLE DE SIMULAÇÃO
  thing["simulation_control"] << [](pson& in) {
    if(!in["mode"].is_empty()) {
      simulationMode = in["mode"];
    }
  };
  
  //CONTROLE DE CALIBRAÇÃO
  thing["calibrate"] << [](pson& in) {
    if(!in["reset"].is_empty() && in["reset"]) {
      waterLevel = 0;
      systemStatus = "NORMAL";
      emergencyAlerted = false;
      simulationMode = 0;
      simulatedDistance = 80.0;
      updateActuators();
      Serial.println("🔄 Sistema calibrado e resetado remotamente");
    }
  };
  
  // === TESTE INICIAL ===
  testSystem();
  
  Serial.println("Sistema inicializado. Gerando dados simulados...");
  Serial.println("Acesse: https://console.thinger.io para monitorar");
}

void loop() {
  thing.handle();
  
  // Atualização de dados e envio para dashboard
  if (millis() - lastMeasurement >= MEASUREMENT_INTERVAL) {
    generateSimulatedData();  
    analyzeRisk();
    updateActuators();
    sendDataToThinger();     
    printStatus();
    lastMeasurement = millis();
  }
  
  delay(100);
}

// === GERAÇÃO DE DADOS SIMULADOS ===
void generateSimulatedData() {
  switch (simulationMode) {
    case 0:
      simulateAutomaticCycle();
      break;
      
    case 1: 
      waterLevel = 30 + random(-5, 5);  
      rainIntensity = random(0, 20);    
      simulatedDistance = 70 + random(-5, 5);
      break;
      
    case 2: 
      waterLevel = 75 + random(-3, 3);  
      rainIntensity = random(40, 60);   
      simulatedDistance = 25 + random(-3, 3);
      break;
      
    case 3: 
      waterLevel = 95 + random(-2, 2);  
      rainIntensity = random(70, 90);   
      simulatedDistance = 5 + random(-2, 2);
      break;
      
    case 4: 
      simulateHeavyRainScenario();
      break;
  }
  
  waterLevel = constrain(waterLevel, 0, 100);
  rainIntensity = constrain(rainIntensity, 0, 100);
  simulatedDistance = constrain(simulatedDistance, 2, 98);
}

// === SIMULAÇÃO AUTOMÁTICA ===
void simulateAutomaticCycle() {
  static unsigned long cycleTimer = 0;
  unsigned long cycleTime = millis() - cycleTimer;
  
  if (cycleTime > 120000) {
    cycleTimer = millis();
    cycleTime = 0;
  }
  
  if (cycleTime < 30000) {
    waterLevel = 25 + random(0, 15);
    rainIntensity = random(0, 25);
    simulatedDistance = 75 + random(-10, 10);
  } 
  else if (cycleTime < 60000) {
    waterLevel = 40 + (cycleTime - 30000) / 1000; 
    rainIntensity = 30 + random(0, 30);
    simulatedDistance = 60 - (cycleTime - 30000) / 1500;
  }
  else if (cycleTime < 90000) {
    waterLevel = 70 + random(0, 15);
    rainIntensity = 50 + random(0, 30);
    simulatedDistance = 30 + random(-5, 5);
  }
  else {
    waterLevel = 90 + random(0, 8);
    rainIntensity = 75 + random(0, 25);
    simulatedDistance = 10 + random(-3, 3);
  }
}

// === SIMULAÇÃO DE CHUVA INTENSA ===
void simulateHeavyRainScenario() {
  static float currentLevel = 30.0;
  
  rainIntensity = 80 + random(0, 20);
  
  if (rainIntensity > 70) {
    currentLevel += random(1, 3);
  } else {
    currentLevel += random(0, 1);
  }
  
  waterLevel = currentLevel;
  if (waterLevel > 98) {
    currentLevel = 30; 
  }
  
  simulatedDistance = WATER_TANK_HEIGHT - (waterLevel * WATER_TANK_HEIGHT / 100);
}

// === ANÁLISE DE RISCO ===
void analyzeRisk() {
  String previousStatus = systemStatus;
  
  if (waterLevel >= EMERGENCY_THRESHOLD) {
    systemStatus = "EMERGENCIA";
  } else if (waterLevel >= ALERT_THRESHOLD) {
    systemStatus = "ALERTA";
  } else {
    systemStatus = "NORMAL";
  }
  
  // Notificação de mudança de status
  if (systemStatus != previousStatus) {
    Serial.println("MUDANÇA DE STATUS: " + previousStatus + " -> " + systemStatus);
    
    // Alerta de emergência
    if (systemStatus == "EMERGENCIA" && !emergencyAlerted) {
      sendEmergencyAlert();
      emergencyAlerted = true;
    } else if (systemStatus != "EMERGENCIA") {
      emergencyAlerted = false;
    }
  }
}

// === ATUALIZAÇÃO DOS ATUADORES ===
void updateActuators() {
  digitalWrite(LED_RED_PIN, LOW);
  digitalWrite(LED_GREEN_PIN, LOW);
  digitalWrite(LED_BLUE_PIN, LOW);
  digitalWrite(BUZZER_PIN, LOW);
  
  if (systemStatus == "NORMAL") {
    digitalWrite(LED_GREEN_PIN, HIGH);
    buzzerActive = false;
  } 
  else if (systemStatus == "ALERTA") {
    digitalWrite(LED_RED_PIN, HIGH);
    digitalWrite(LED_GREEN_PIN, HIGH); // Amarelo
    buzzerActive = false;
  } 
  else if (systemStatus == "EMERGENCIA") {
    digitalWrite(LED_RED_PIN, HIGH);
    buzzerActive = true;
    
    // Buzzer intermitente
    static unsigned long lastBuzzer = 0;
    if (millis() - lastBuzzer >= 500) {
      digitalWrite(BUZZER_PIN, !digitalRead(BUZZER_PIN));
      lastBuzzer = millis();
    }
  }
}

// === ENVIO CONTÍNUO PARA THINGER.IO ===
void sendDataToThinger() {
  if (WiFi.status() == WL_CONNECTED) {
    // Envia dados sempre que chamada (a cada atualização)
    thing.stream("sensor_data");
    thing.stream("water_level");
    thing.stream("rain_intensity");
    thing.stream("system_status");
    
    Serial.println("Dados enviados para Thinger.io Dashboard");
  } else {
    Serial.println("📡 WiFi desconectado - tentando reconectar...");
    WiFi.begin(SSID, SSID_PASSWORD);
  }
}

// === ALERTA DE EMERGÊNCIA ===
void sendEmergencyAlert() {
  Serial.println("🚨🚨🚨 ALERTA DE EMERGÊNCIA! 🚨🚨🚨");
  Serial.println("💧 Nível crítico atingido: " + String(waterLevel, 1) + "%");
  
  // Stream imediato de emergência
  if (WiFi.status() == WL_CONNECTED) {
    thing.stream("sensor_data");
    Serial.println("📡 ALERTA DE EMERGÊNCIA enviado para Thinger.io");
  } else {
    Serial.println("⚠️ WiFi desconectado - alerta local ativo");
  }
}

// === STATUS DETALHADO ===
void printStatus() {
  Serial.println("================================================");
  Serial.println("     SISTEMA DE MONITORAMENTO DE ENCHENTES     ");
  Serial.println("================================================");
  Serial.println("🌊 Nível da Água: " + String(waterLevel, 1) + "% (" + getStatusEmoji() + ")");
  Serial.println("🌧️ Intensidade Chuva: " + String(rainIntensity) + "% (" + getRainEmoji() + ")");
  Serial.println("📏 Distância Simulada: " + String(simulatedDistance, 1) + " cm");
  Serial.println("📊 Status do Sistema: " + systemStatus + " " + getStatusEmoji());
  
  // LEDs e Buzzer
  Serial.println("💡 LEDs: R=" + String(digitalRead(LED_RED_PIN) ? "🔴" : "⚫") + 
                 " G=" + String(digitalRead(LED_GREEN_PIN) ? "🟢" : "⚫") + 
                 " B=" + String(digitalRead(LED_BLUE_PIN) ? "🔵" : "⚫"));
  Serial.println("🔊 Buzzer: " + String(buzzerActive ? "🔊 ATIVO" : "🔇 INATIVO"));
  
  // Status da conexão
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("📶 WiFi: CONECTADO ✅ (" + WiFi.localIP().toString() + ")");
    Serial.println("📱 Thinger.io: " + String(thing.is_connected() ? "CONECTADO ✅" : "CONECTANDO ⏳"));
  } else {
    Serial.println("📶 WiFi: DESCONECTADO ❌");
    Serial.println("📱 Thinger.io: OFFLINE");
  }
  
  Serial.println("⏰ Uptime: " + String(millis() / 1000) + "s");
  Serial.println("================================================\n");
}

// === STATUS DA CHUVA COR ===
String getStatusEmoji() {
  if (systemStatus == "NORMAL") return "🟢";
  if (systemStatus == "ALERTA") return "🟡";
  if (systemStatus == "EMERGENCIA") return "🔴";
  return "❓";
}

String getRainEmoji() {
  if (rainIntensity < 30) return "🌦️";
  if (rainIntensity < 70) return "🌧️";
  return "⛈️";
}

String getSimulationModeText() {
  switch (simulationMode) {
    case 0: return "Automático";
    case 1: return "Normal";
    case 2: return "Alerta";
    case 3: return "Emergência";
    case 4: return "Chuva Intensa";
    default: return "Desconhecido";
  }
}

// === TESTE INICIAL ===
void testSystem() {
  Serial.println("\n=== TESTE INICIAL DO SISTEMA ===");
  
  Serial.println("Testando LEDs...");
  digitalWrite(LED_RED_PIN, HIGH); delay(300); digitalWrite(LED_RED_PIN, LOW);
  digitalWrite(LED_GREEN_PIN, HIGH); delay(300); digitalWrite(LED_GREEN_PIN, LOW);
  digitalWrite(LED_BLUE_PIN, HIGH); delay(300); digitalWrite(LED_BLUE_PIN, LOW);
  
  Serial.println("Testando buzzer...");
  digitalWrite(BUZZER_PIN, HIGH); delay(200); digitalWrite(BUZZER_PIN, LOW);
  
  Serial.println("Inicializando dados simulados...");
  waterLevel = 25.0;
  rainIntensity = 10;
  simulatedDistance = 75.0;
  
  Serial.println("TESTE CONCLUÍDO! Sistema pronto!\n");
}

