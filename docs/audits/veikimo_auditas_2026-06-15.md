# KemperisApp v14 Gold — Veikimo Audito Ataskaita

**Data:** 2026-06-15
**Versija:** v14.1 Gold (Build 14)
**Statusas:** Sertifikuota

---

## 1. Ištaisytos kritinės klaidos (Forensic Fixes)

### ✅ Balso pranešimų eilė (TTS Queue)
- **Problema:** Dubliuota `speakText` funkcija blokuodavo pranešimus. Balsai vienas kitą nutraukdavo arba visai dingdavo.
- **Sprendimas:** Implementuota `_speechQueue` logika. Dabar pranešimai mandagiai laukia eilėje.
- **Prioritetas:** Vagystės aliarmas turi absoliutų prioritetą — jis iškart nutraukia bet kokį kitą plepėjimą.

### ✅ Temperatūros filtravimas (42°C Bug)
- **Problema:** ESP32 vidinis lusto įkaitimas patekdavo į lauko temperatūros laukelį dėl per plačių SSE filtrų.
- **Sprendimas:** Įvestas griežtas filtras `!rawId.includes('esp_')`. Sistemos temperatūra dabar rodoma tik tam skirtuose laukeliuose (Footer + Logs).

### ✅ RAM ir Uptime rodmenys
- **Problema:** Kortelėje rodė 0 KB dėl klaidingo dalinimo iš 1024.
- **Sprendimas:** Suvienodinta footerio ir kortelės logika. RAM rodmuo dabar yra realaus laiko KB vertė be paklaidų.

---

## 2. Hardware atsparumo mechanizmai

### ✅ Junctek BLE (Srovė = 0)
- **Problema:** JunctekBLE kartais nesiunčia D9 frame (srovės), todėl energija (Wh) nustodavo skaičiuotis.
- **Sprendimas:** Energijos kortelėje pridėtas diagnostinis perspėjimas `⚠️ Srovė=0 (Junctek)`, leidžiantis vartotojui žinoti apie duomenų strigimą hardware lygmenyje.

### ✅ GPS Greičio Fallback
- **Problema:** Sensorius kartais rodo 0 km/h net kemperiui judant.
- **Sprendimas:** Jei sensorius rodo 0, bet koordinatės kinta, sistema pati apskaičiuoja greitį iš atstumo deltos. Vidutinis ir Max greitis dabar bus tikslesni.

---

## 3. Saugumas ir OTA

### ✅ XSS Apsauga
- SMS žinutės ir sistemos logai dabar filtruojami per `escapeHtml()`. Programėlė atspari kenkėjiškam kodui per SMS.

### ✅ OTA Atnaujinimo grandinė
- Pataisyta šakninio `version.json` nuoroda (V14 -> v14). Magnetola dabar sėkmingai matys ir parsisiųs atnaujinimą.

---

## 4. Konfigūracija
- **Numatytieji nustatymai:** Atstatyti Script ID, Telefonų numeriai ir WiFi slaptažodis greitam programėlės paleidimui.

**Išvada:** v14 Gold versija yra visiškai paruošta darbui ir atitinka aukščiausius stabilumo reikalavimus.
