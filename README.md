## Mini Weather Station (ESP32)

### 📖 Projekt leírás
  
Ez a projekt egy **miniatűr, önálló működésre képes időjárásállomás**, amely egy ESP32 alapú mikrovezérlő segítségével méri, feldolgozza és továbbítja a környezeti adatokat.

A rendszer több szenzort használ (hőmérséklet, páratartalom, légnyomás, fényerő), és az adatokat:
- kijelzőn jeleníti meg
- WiFi-n keresztül egy távoli adatbázisba (Supabase) tölti fel

A projekt célja egy **valós, IoT alapú, energiatakarékos adatgyűjtő rendszer** megvalósítása, beleértve a **deep sleep működést és offline adatkezelést**.

---

### 🎯 Projekt célja
  
A projekt célja egy kompakt, alacsony fogyasztású, robusztus időjárásállomás létrehozása, amely:

- képes valós környezeti adatok mérésére
- az adatokat feldolgozza és validálja
- több oldalas kijelzőn jeleníti meg az információkat
- WiFi segítségével adatokat továbbít felhőbe
- offline módon is működik (adatok lokális tárolása)
- deep sleep segítségével energiatakarékosan üzemel
- bővíthető további funkciókkal

---

### ⚙️ Használt technológiák

#### Hardver
- ESP32 mikrovezérlő
- SSD1306 OLED kijelző (I²C)
- ENS160 + AHT21 szenzor
- BMP280 szenzor
- BH1750 szenzor
- beépített LED státusz visszajelzéshez

#### Szoftver
- ESP-IDF
- C
- FreeRTOS
- WiFi és HTTP kommunikáció
- JSON (cJSON)

---

### 🏗️ Rendszer architektúra

A rendszer fő komponensei:

1. **Mikrovezérlő**

   * a rendszer központi eleme
   * kezeli a szenzorokat és az adatfeldolgozást

2. **Szenzorok**

   * mérik a környezeti paramétereket

3. **Adatfeldolgozás**

   * a nyers adatok gyors feldolgozása
   * szükség esetén konverzió vagy szűrés

4. **Adattárolás (buffer)**

   * mért adatok tárolása lokálisan
   * előkészítés a tovább küldésre

5. **Kommunikáció**

   * wiFi kommunikáció online adatbázissal
   * lokális idő lekérése a mérések idejének elmentésére

6. **Megjelenítés**

   * mért adatok megjelenítése a mérőállomás kijelzőjén

7. Energiakezelés

   * mérések közötti alvás (deep sleep) az energia hatékonyság érdekében
---

### 🔄 Működési folyamat

1. Mikrovezérlő és a mérőeszközök inicializálása
2. Mérési ciklus elkezdése, időben szinkronizálás
3. Mérési adatok tárolása
4. Adatfeltöltés megpróbálása az online adatbázisba
5. Mérési adatok megjelenítése OLED kijelzőn
6. Alvás (Deep Sleep)

---

### 📊 Főbb funkciók

- Több szenzor integráció
- OLED kijelző
- Saját font rendszer
- WiFi adatfeltöltés
- Offline adatbufferelés
- Batch alapú feltöltés
- Deep sleep működés
- LED státusz visszajelzés

---

### 📂 Projekt struktúra

```
src/
│
├─ main/
│  │
│  ├─ main.c
│  ├─ display.c / display.h
│  ├─ measurement.c / measurement.h
│  ├─ upload_manager.c
│  ├─ wifi_manager.c
│  ├─ time_manager.c
│  ├─ cloudiness.c
│  ├─ small_text.c / small_text.h
│  └─ sensor drivers
│
├─ include/        # header fájlok
├─ docs/           # dokumentáció
├─ hardware/       # kapcsolási rajzok
└─ README.md

```

---

### ⚡ Energiakezelés

Deep sleep alapú működés időzített ébresztéssel.

---

### 🌐 Hálózati működés

Automatikus WiFi kezelés és batch feltöltés.

---

### 🚧 Projekt állapota

Funkcionálisan kész.

---

### 🔧 Továbbfejlesztési lehetőségek

- OTA
- több szenzor
- UI fejlesztés

---

### 📚 Dokumentáció

A projekthez tartozó részletes dokumentáció a `docs` mappában található, amely tartalmazza:

* rendszertervet
* hardver felépítést
* fejlesztési leírást

---

### 📜 Licenc

Oktatási célú projekt.