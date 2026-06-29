#include <Arduino.h>
#include "driver/twai.h"

#define CAN_TX_PIN 20
#define CAN_RX_PIN 21

// OBD-II PID, kuriuos norime aktyviai užklausti
const uint8_t pids[] = {0x0C, 0x0D, 0x05, 0x04, 0x0B, 0x10}; 
const char* pidNames[] = {"RPM", "Speed", "Coolant", "Load", "MAP", "MAF"};
const int numPids = sizeof(pids) / sizeof(pids[0]);

void setup() {
  Serial.begin(115200);
  while (!Serial);

  twai_general_config_t g_config = TWAI_GENERAL_CONFIG_DEFAULT((gpio_num_t)CAN_TX_PIN, (gpio_num_t)CAN_RX_PIN, TWAI_MODE_NORMAL);
  twai_timing_config_t t_config = TWAI_TIMING_CONFIG_500KBITS();
  twai_filter_config_t f_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();

  if (twai_driver_install(&g_config, &t_config, &f_config) == ESP_OK) {
    twai_start();
  } else {
    Serial.println("CAN init failed");
    while (1) {}
  }
  delay(1000); 
}

void sendOBDPid(uint8_t pid) {
  twai_message_t msg;
  msg.identifier = 0x7DF;          
  msg.extd = 0;
  msg.data_length_code = 8;
  msg.data[0] = 0x02;              
  msg.data[1] = 0x01;              
  msg.data[2] = pid;
  msg.data[3] = 0xAA; msg.data[4] = 0xAA; msg.data[5] = 0xAA; msg.data[6] = 0xAA; msg.data[7] = 0xAA;
  twai_transmit(&msg, pdMS_TO_TICKS(10));
}

float parsePIDResponse(uint8_t pid, uint8_t A, uint8_t B) {
  switch (pid) {
    case 0x0C: return (A * 256 + B) / 4.0;   // RPM
    case 0x0D: return A;                     // Greitis km/h
    case 0x05: return A - 40.0;              // Aušinimo skystis °C
    case 0x04: return A * 100.0 / 255.0;     // Apkrova %
    case 0x0B: return A;                     // Įsiurbimo slėgis kPa
    case 0x10: return (A * 256 + B) / 100.0; // Oro srautas g/s
    default:   return A;
  }
}

unsigned long lastOBDRequest = 0;
const unsigned long obdInterval = 500; 
int currentPidIndex = 0;

void loop() {
  unsigned long now = millis();

  // 1. Siunčiame užklausas be jokio laukimo ciklo (non-blocking)
  if (now - lastOBDRequest >= obdInterval) {
    lastOBDRequest = now;
    sendOBDPid(pids[currentPidIndex]);
    currentPidIndex = (currentPidIndex + 1) % numPids;
  }

  // 2. Skaitome visus ateinančius kadrus, kol buferis ištuštėja
  twai_message_t rawMsg;
  while (twai_receive(&rawMsg, 0) == ESP_OK) { // Laukimas 0 ms!
    
    // A. Jei tai atsakymas iš variklio kompiuterio (0x7E8) į mūsų OBD užklausą
    if (!rawMsg.extd && rawMsg.identifier == 0x7E8 && rawMsg.data_length_code >= 5) {
      uint8_t mode = rawMsg.data[1];
      uint8_t pid = rawMsg.data[2];
      
      if (mode == 0x41) { // Patvirtinimas, kad tai atsakymas į 0x01 užklausą
        for (int i = 0; i < numPids; i++) {
          if (pids[i] == pid) {
            float value = parsePIDResponse(pid, rawMsg.data[3], rawMsg.data[4]);
            Serial.print("OBD,");
            Serial.print(pidNames[i]);
            Serial.print(",");
            Serial.println(value);
            break;
          }
        }
      }
    }

    // B. Bet kokiu atveju atspausdiname VISĄ srautą programuotojams (RAW)
    Serial.print("RAW,");
    Serial.print(millis());
    Serial.print(",");
    if (rawMsg.extd) {
      Serial.print("E,");
      if (rawMsg.identifier < 0x10000000) Serial.print("0");
      if (rawMsg.identifier < 0x1000000) Serial.print("0");
      if (rawMsg.identifier < 0x100000) Serial.print("0");
      if (rawMsg.identifier < 0x10000) Serial.print("0");
      Serial.print(rawMsg.identifier, HEX);
    } else {
      Serial.print("S,");
      if (rawMsg.identifier < 0x100) Serial.print("0");
      if (rawMsg.identifier < 0x10) Serial.print("0");
      Serial.print(rawMsg.identifier, HEX);
    }
    Serial.print(",");
    Serial.print(rawMsg.data_length_code);
    Serial.print(",");
    for (int i = 0; i < rawMsg.data_length_code; i++) {
      if (rawMsg.data[i] < 0x10) Serial.print("0");
      Serial.print(rawMsg.data[i], HEX);
    }
    Serial.println();
  }
}