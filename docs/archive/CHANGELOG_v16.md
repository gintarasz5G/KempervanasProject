# Changelog: kempervanas_v15 → v16

**Datą:** 2026-05-08  
**Tikslas:** Išspręsti Junctek KH-F150A BLE parsing problemą (voltage/current mismatch)

---

## 🔴 Idetifikuota Problema

### Symptomatoriai:
- **Hardware ekrane:** 14.19V, 3.70A
- **Web Dashboard:** 11.21V, 0.5A
- **Skirtumas:** -3.0V, -3.2A

### Root Cause:
**Junctek BLE Lambda (eilutės 767-830) neatpažino 0xC0 (voltage) ir 0xC1 (current) registro markierių**

#### Detalus Paaiškinimas:

1. **Junctek BLE frame'o standartinis formatas:**
   ```
   0xBB [0xC0 V1 V2] [0xC1 A1 A2] [0xD5...] [0xD6...] [0xD8...] 0xEE
   └─┬──┘  └──────┬──────┘  └──────┬──────┘
     Start  Voltage (BCD)  Current (BCD)
   ```

2. **Esamoje (v15) konfigūracijoje:**
   - Buvo paraštas kodas, kuris **neatpažino** 0xC0 ir 0xC1
   - Bandymas dekoduoti voltage iš `buf[1]` ir `buf[2]` be patikrinimo, ar tai tikrai voltage
   - Sąlyga `if (buf[1] == 0x13 || buf[1] == 0x10)` buvo **tik atsitiktinis klaidosūkis**
   - Srovė buvo skaičiuojama iš (Power ÷ Voltage), o kadangi voltai klaidingi → ir srovė klaidinga

---

## ✅ Pataisymai — v16

### Pagrindinė Taista (eilutės 767-832):

#### ❌ BUVO (v15):
```cpp
// Įtampa: BCD dekodavimas iš 1-2 baito (buf[1]=dešimtys, buf[2]=šimtosios)
if (n >= 5 && (buf[1] == 0x13 || buf[1] == 0x10)) {  // ← KLAIDINGA SĄLYGA!
  float v = (buf[1] >> 4) * 10 + (buf[1] & 0x0F) + ...
  id(junc_v).publish_state(v);
}

// Registro markierių parsavimas (0xD5, 0xD6, 0xD8) — BET NE 0xC0/0xC1!!!
if (m == 0xD5 && ...) { ... }
else if (m == 0xD6) { 
  // ...
  // Srovė = Galia ÷ Įtampa  ← KLAIDINGA JEI VOLTAI NETEISŪS!
  id(junc_a).publish_state(p / id(junc_v).state);
}
```

#### ✅ DABAR (v16):
```cpp
// PRIDĖTI 0xC0 (voltage) IR 0xC1 (current) MARKERIAI:
if (m == 0xC0 && byte_count >= 2) {
  float v = (val / 100.0f);  // 1419 BCD → 14.19V ✅
  id(junc_v).publish_state(v);
} else if (m == 0xC1 && byte_count >= 2) {
  float a = (val / 100.0f);  // 0370 BCD → 3.70A ✅
  id(junc_a).publish_state(a);
}
// ... D5, D6, D8 lieka beveik nepakitę
```

### Specifiniai Kodų Pokyčiai:

| Eilutės | Pokytis | Priežastis |
|---------|---------|-----------|
| 769-771 | Atnaujinti komentarai | Dokumentuoti 0xC0, 0xC1 įtraukimą |
| 783 | Pridėti 0xC0, 0xC1 į komentarą | Aiškiai nurodyti kuriuos markierius parsavome |
| 786 | Pridėti `byte_count` global | Sekti kiek baitų po markierio (garantuoti 2) |
| 791-799 | **PRIDĖTA NAUJA LOGIKA** | 0xC0 ir 0xC1 decodavimas |
| 800-802 | D5 nekeitėm | Lieka tokia pat |
| 803-805 | D6 šalintas `/ id(junc_v)` | **Srovė dabar iš 0xC1, ne iš Power/V** |
| 815 | `for (size_t i = 1; ...)` vietoj `i = 2` | Nepraleiškime pirmojo markierio po 0xBB |
| 825-826 | `byte_count++` po BCD | Sekti baitų kiekį per markerį |

---

## 📊 Numatyti Rezultatai

### Prieš Pataisą (v15):
```
Hardware (Junctek device):    14.19V   3.70A   
Web Dashboard (kempervanas):   11.21V   0.5A  ❌
```

### Po Pataisos (v16):
```
Hardware (Junctek device):    14.19V   3.70A   
Web Dashboard (kempervanas):  14.19V   3.70A  ✅
```

---

## 🔧 Techninė Detalė: BCD Dekodavimas

### Pavyzdys: Įtampa 14.19V

```
Frame: 0xBB ... 0xC0 0x14 0x19 ... 0xEE

Baitas 0x14 (BCD):           Baitas 0x19 (BCD):
  0x14 = 0001 0100           0x19 = 0001 1001
       = 1 4                       = 1 9
  
Dekodavimas (lambda):        Dekodavimas (lambda):
  upper = 1 (dešimtys)       upper = 1 (dešimtosios)
  lower = 4 (vienetai)       lower = 9 (šimtosios)
  
Akumuliavimas:
  i=0: val = 0*100 + (1*10+4) = 14
  i=1: val = 14*100 + (1*10+9) = 1419
  
Konverzija: 1419 / 100 = 14.19V ✅
```

### Pavyzdys: Srovė 3.70A

```
Frame: 0xBB ... 0xC1 0x03 0x70 ... 0xEE

Baitas 0x03 (BCD):           Baitas 0x70 (BCD):
  0x03 = 0000 0011           0x70 = 0111 0000
       = 0 3                       = 7 0

Akumuliavimas:
  i=0: val = 0*100 + (0*10+3) = 3
  i=1: val = 3*100 + (7*10+0) = 370

Konverzija: 370 / 100 = 3.70A ✅
```

---

## 📚 Šaltiniai & Pagrindimas

- **jowser7/Junctek_BLE** — HomeAssistant integr., atverčia 0xC0/0xC1 markierius
- **chriskomus/ble-sniffer-walkthrough** — BLE reverse engineer, apibūdina markierius
- **JUNCTEK KH-F Series User Manual** — oficialus formatas (jei prieinamas)

---

## ⚠️ Atgalinė Kompatibilumas

- **OTA**: Nereikalingas hard reset. Įrenginys persivaldys.
- **NVS**: Visi Calibration parametrai išliks (cal_roll, cal_pitch, etc.)
- **Web UI**: Nereikalingas refresh — SSE atsinaujins automatiškai

---

## 📋 Baigtinai Įvertinti Prieš Commit'inimą

- [ ] Patikrinti sintaksę: `esphome config kempervanas_v15.yaml`
- [ ] Kompiuoti: `esphome compile kempervanas_v15.yaml`
- [ ] Flash: `esphome run kempervanas_v15.yaml`
- [ ] Patikrinti web dashboard — **14.19V** (ne 11.21V)
- [ ] Patikrinti logus — bez parsing klaidų
- [ ] Lyginimas prieš/po — voltage/current now match hardware

---

## 🔄 Jei Problema Išlieka

Žr. **JUNCTEK_TEST_PLAN.md** — 6 žingsniai debugging'o protokolo.

