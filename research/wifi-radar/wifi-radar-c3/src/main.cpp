#include <Arduino.h>
#include <WiFi.h>
#include <esp_wifi.h>
#include <BLEDevice.h>
#include <BLEScan.h>

const char* ssid = "Telia-D41F43-Greitas";
const char* password = "AEB6A6B8B0";

// === Tapatybės (VW Crafter Ekipažas) ===
const uint8_t mac_guoda[6]    = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00}; 
const uint8_t mac_gintaras[6] = {0xc8, 0x2b, 0x96, 0xe9, 0x04, 0xe6};

// === Ignoruojami įrenginiai (TV, kolonėlės ir kiti stacionarūs daiktai) ===
const uint8_t mac_ignored[6]  = {0x20, 0x23, 0x51, 0x06, 0xc6, 0x5a}; 

// === Būsenos ir Atmintis ===
int rssi_guoda = -100, prev_guoda = -100;
int rssi_gintaras_phone = -100, prev_gintaras_phone = -100;
int rssi_gintaras_watch = -100, prev_gintaras_watch = -100;

// Svečių masyvas
String guest_macs[5];
int guest_rssis[5];
int guest_count = 0;

// === CSI Kintamieji (Judesys) ===
volatile long csi_delta_max = 0;
volatile long csi_last_sum = 0;
volatile long csi_variation = 0;

TaskHandle_t RadarTaskHandle;

float calculateDist(int rssi) {
    if (rssi <= -95 || rssi == 0) return 0.0;
    return pow(10, (float)(-45 - rssi) / (10 * 2.8));
}

// 1. CSI FIZINIO JUDESIO CALLBACK
void csi_callback(void *ctx, wifi_csi_info_t *info) {
    if (!info || !info->buf) return;
    int8_t *data = (int8_t *)info->buf;
    long sum = 0;
    for (int i = 0; i < info->len; i++) sum += abs(data[i]);
    
    long delta = abs(sum - csi_last_sum);
    if (delta > csi_delta_max) csi_delta_max = delta;
    csi_variation = (csi_variation + delta) / 2; 
    csi_last_sum = sum;
}

// 2. WIFI SNIFFER CALLBACK
void sniffer_callback(void* buf, wifi_promiscuous_pkt_type_t type) {
    if (type != WIFI_PKT_MGMT && type != WIFI_PKT_DATA) return;
    wifi_promiscuous_pkt_t *pkt = (wifi_promiscuous_pkt_t *)buf;
    int rssi = pkt->rx_ctrl.rssi;
    uint8_t* p = pkt->payload;

    auto is_match = [&](const uint8_t* target) {
        for(int i=0; i<6; i++) if(p[10+i] != target[i]) return false;
        return true;
    };

    // Jei tai ignoruojamas prietaisas - iškart baigiame darbą
    if (is_match(mac_ignored)) return;

    if (is_match(mac_guoda)) {
        if (rssi > rssi_guoda) rssi_guoda = rssi;
    } else if (is_match(mac_gintaras)) {
        if (rssi > rssi_gintaras_phone) rssi_gintaras_phone = rssi;
    } else if (rssi > -85) { 
        char m[18]; sprintf(m, "%02x:%02x:%02x:%02x:%02x:%02x", p[10],p[11],p[12],p[13],p[14],p[15]);
        String macStr = String(m);
        bool found = false;
        for(int i=0; i<guest_count; i++) {
            if(guest_macs[i] == macStr) {
                if(rssi > guest_rssis[i]) guest_rssis[i] = rssi;
                found = true; break;
            }
        }
        if(!found && guest_count < 5) {
            guest_macs[guest_count] = macStr;
            guest_rssis[guest_count] = rssi;
            guest_count++;
        }
    }
}

// 3. PAGRINDINIS RADARO CIKLAS (CORE 0)
void radarLoop(void * parameter) {
    for(;;) {
        rssi_guoda = -100; rssi_gintaras_phone = -100; rssi_gintaras_watch = -100;
        guest_count = 0; csi_delta_max = 0;

        vTaskDelay(2000 / portTICK_PERIOD_MS); 
        bool motion_detected = (csi_delta_max > 120);

        BLEScan* scan = BLEDevice::getScan();
        BLEScanResults* results = scan->start(2, false);
        for (int i=0; i < results->getCount(); i++) {
            BLEAdvertisedDevice d = results->getDevice(i);
            if (String(d.getName().c_str()).indexOf("Amazfit") >= 0) rssi_gintaras_watch = d.getRSSI();
        }
        scan->clearResults();

        esp_wifi_set_promiscuous(true);
        esp_wifi_set_promiscuous_rx_cb(&sniffer_callback);
        for(int ch=1; ch<=11; ch++) { 
            esp_wifi_set_channel(ch, WIFI_SECOND_CHAN_NONE); 
            vTaskDelay(150 / portTICK_PERIOD_MS); 
        }
        esp_wifi_set_promiscuous(false);

        WiFi.disconnect(); 
        WiFi.begin(ssid, password);
        while (WiFi.status() != WL_CONNECTED) vTaskDelay(10 / portTICK_PERIOD_MS);

        auto isMoving = [](int cur, int prev) { return (cur > -95) && (abs(cur - prev) >= 5); };
        bool guoda_m = isMoving(rssi_guoda, prev_guoda);
        bool gintaras_m = isMoving(rssi_gintaras_phone, prev_gintaras_phone) || isMoving(rssi_gintaras_watch, prev_gintaras_watch);
        
        int g_rssi = (rssi_gintaras_phone > rssi_gintaras_watch) ? rssi_gintaras_phone : rssi_gintaras_watch;
        
        bool any_guest_moving = false;
        for(int i=0; i<guest_count; i++) {
            if(guest_rssis[i] > -85) any_guest_moving = true; 
        }

        bool person_without_phone = motion_detected && !gintaras_m && !guoda_m && !any_guest_moving;

        int estimated_people = 0;
        if (g_rssi > -95) estimated_people++;
        if (rssi_guoda > -95) estimated_people++;
        estimated_people += guest_count;
        if (person_without_phone) estimated_people++;

        // IŠVEDIMAS Į TERMINALĄ (Su lėtesniu buferio atidavimu)
        Serial.println("\n==========================================");
        Serial.println(">>> ESP32-S3 KEMPERIO RADARAS (CORE 0) <<<");
        Serial.println("------------------------------------------");
        vTaskDelay(10 / portTICK_PERIOD_MS); 

        Serial.printf("1. Eterio variacija (CSI): %ld\n", csi_variation);
        Serial.printf("2. Judesio intensyvumas:   %s (Pikas: %ld)\n", motion_detected ? "FIKSUOTAS" : "RAMYBĖ", csi_delta_max);
        Serial.printf("3. Apskaičiuotas objektų skaičius: ~%d\n", estimated_people);
        Serial.println("------------------------------------------");
        vTaskDelay(10 / portTICK_PERIOD_MS);

        Serial.println("[ EKIPAŽAS ]");
        if(g_rssi > -95) Serial.printf("  └─ GINTARAS:  ~%.1f m %s\n", calculateDist(g_rssi), gintaras_m ? "[*JUDA*]" : "");
        else Serial.println("  └─ GINTARAS:  (Nėra)");
        
        if(rssi_guoda > -95) Serial.printf("  └─ GUODA:     ~%.1f m %s\n", calculateDist(rssi_guoda), guoda_m ? "[*JUDA*]" : "");
        else Serial.println("  └─ GUODA:     (Nėra)");
        vTaskDelay(10 / portTICK_PERIOD_MS);

        Serial.printf("\n[ SVEČIAI / NEŽINOMI ĮRENGINIAI: %d ]\n", guest_count);
        for(int i=0; i<guest_count; i++) {
            Serial.printf("  └─ MAC: %s | ~%.1f m\n", guest_macs[i].c_str(), calculateDist(guest_rssis[i]));
            vTaskDelay(5 / portTICK_PERIOD_MS); 
        }

        Serial.println("------------------------------------------");
        Serial.print("! IDENTIFIKACIJA: ");
        if (motion_detected) {
            if (person_without_phone) Serial.println("JUDĖJIMAS BE TELEFONO (Gyvūnas, siurblys, svečias)!");
            else {
                String m = "";
                if (gintaras_m) m += "Gintaras, ";
                if (guoda_m) m += "Guoda, ";
                if (any_guest_moving) m += "Svečias, ";
                if (m.length() > 0) { m.remove(m.length()-2); Serial.println(m + " vaikšto."); }
                else Serial.println("Nežymus mikro-judesys.");
            }
        } else {
            Serial.println("Visi ramūs.");
        }
        Serial.println("==========================================\n");

        // Išsaugome sekančiam ciklui
        prev_guoda = rssi_guoda; prev_gintaras_phone = rssi_gintaras_phone; prev_gintaras_watch = rssi_gintaras_watch;
    }
}

// 4. SETUP (Konfigūracija)
void setup() {
    Serial.begin(115200);
    delay(1000);
    
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) delay(500);

    // CSI Konfigūracija
    wifi_csi_config_t config = {.lltf_en=true, .htltf_en=true, .stbc_htltf2_en=true, .ltf_merge_en=true, .channel_filter_en=true, .manu_scale=false, .shift=false};
    esp_wifi_set_csi_config(&config);
    esp_wifi_set_csi_rx_cb(&csi_callback, NULL);
    esp_wifi_set_csi(true);
    
    BLEDevice::init("");

    // --- Kuriame atskirą užduotį ant CORE 0 ---
    xTaskCreatePinnedToCore(
        radarLoop,       // Funkcija, kurią vykdys
        "RadarTask",     // Užduoties pavadinimas
        10000,           // Atminties dydis baitais
        NULL,            // Parametrai
        1,               // Prioritetas
        &RadarTaskHandle,// Rankena
        0                // Priskirta CORE 0
    );
    
    Serial.println("Radaras sekmingai paleistas ant CORE 0!");
}

// 5. PAGRINDINIS CIKLAS (CORE 1)
void loop() {
    vTaskDelay(10 / portTICK_PERIOD_MS);
}