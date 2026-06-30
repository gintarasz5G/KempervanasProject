# Kempervanas v20 - Išleidimo pastabos

Ši versija ištaiso kritinę atnaujinimų tikrinimo klaidą ir sutvarko vandens lygio rankinį atnaujinimą.

## Pakeitimai

### 🚀 Atnaujinimų (OTA) pataisa
* **Ištaisyta sintaksės klaida:** Naujose eilutėse (newlines) aprašymuose laužydavo programėlės kodą, todėl patikra „užtrukdavo“. Dabar visi duomenys perduodami saugiu JSON formatu.
* **Padidintas stabilumas:** Ryšio laukimo laikas (timeouts) padidintas iki 15s, pridėtas „User-Agent“ saugesniam bendravimui su GitHub serveriais.

### 💧 Vandens lygio pataisa (K1)
* Sutvarkytas rankinio atnaujinimo mygtuko kelias (`resursai___vanduo_lygis__`). Dabar paspaudus 🔄 piktogramą vandens kortelėje, duomenys sėkmingai parsiunčiami (nebebus 404 klaidos).

### 🔢 Versijos atnaujinimas
* Sistemos versija pakelta į **20**.
* Sugeneruotas ir įkeltas `kemperis_v20.apk`.

## Diegimo instrukcija
Kadangi v19 versijoje atnaujinimų patikra galėjo strigti, **v20 rekomenduojama įsidiegti rankiniu būdu** vieną paskutinį kartą. Visi ateities atnaujinimai veiks sklandžiai.

👉 [Atsisiųsti kemperis_v20.apk](https://github.com/gintarasz5G/KempervanasProject/raw/main/android/kemperis_v20.apk)

---
Data: 2026-06-30
Versija: v20 (Stability & Fixes)
