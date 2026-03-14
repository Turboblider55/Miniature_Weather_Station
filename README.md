# Mini Weather Station (ESP32)

## 📖 Projekt leírás

Ez a projekt egy **miniatűr időjárásállomás** fejlesztését célozza, amely egy ESP32 alapú mikrovezérlő segítségével méri és dolgozza fel a környezeti adatokat.
A rendszer különböző szenzorok segítségével gyűjti az adatokat (például hőmérséklet, páratartalom, légnyomás), majd ezeket feldolgozza és opcionálisan megjeleníti vagy továbbítja.

A projekt egyetemi célból készült, és célja a **beágyazott rendszerek, szenzorok és IoT alapú adatgyűjtés** gyakorlati megvalósításának bemutatása.

---

## 🎯 Projekt célja

A projekt célja egy kompakt, alacsony fogyasztású időjárásállomás létrehozása, amely:

* képes környezeti adatok mérésére
* feldolgozza a szenzorokból érkező adatokat
* lehetőséget biztosít az adatok megjelenítésére és továbbítására
* bővíthető további szenzorokkal vagy funkciókkal

A projekt emellett demonstrálja a mikrovezérlők és szenzorok együttműködését egy valós alkalmazásban.

---

## ⚙️ Használt technológiák

### Hardver

* ESP32 mikrovezérlő
* Környezeti szenzor(ok)
* opcionális kijelző
* tápellátás (USB vagy akkumulátor)

### Szoftver

* Arduino / ESP-IDF alapú fejlesztés
* C / C++ programozás
* szenzor könyvtárak
* WiFi kommunikáció

---

## 🏗️ Rendszer architektúra

A rendszer fő komponensei:

1. **Mikrovezérlő**

   * a rendszer központi eleme
   * kezeli a szenzorokat és az adatfeldolgozást

2. **Szenzorok**

   * mérik a környezeti paramétereket

3. **Adatfeldolgozás**

   * a nyers adatok gyors feldolgozása
   * szükség esetén konverzió vagy szűrés

4. **Adatkimenet**

   * kijelzőn való megjelenítés
   * WiFi-n keresztüli továbbítás távoli szerver felé

---

## 🔄 Működési folyamat

A rendszer működése a következő lépésekből áll:

1. A mikrovezérlő inicializálja a szenzorokat
2. A rendszer meghatározott időközönként kiolvassa a szenzoradatokat
3. Az adatok feldolgozásra kerülnek
4. A feldolgozott adatok megjelennek vagy továbbításra kerülnek
5. A rendszer új mérési ciklust indít

---

## 📂 Projekt struktúra

```
project-root
│
├─ src/            # forráskód
├─ include/        # header fájlok
├─ docs/           # dokumentáció
├─ hardware/       # kapcsolási rajzok
└─ README.md
```

---

## 🚧 Projekt állapota

A projekt jelenleg **fejlesztés alatt áll**.
A következő lépések vannak tervben:

* hardver komponensek kiválasztása
* kapcsolási rajz elkészítése
* szenzorok integrálása
* adatgyűjtő program fejlesztése
* rendszer tesztelése

---

## 🔧 Továbbfejlesztési lehetőségek

A projekt később az alábbi funkciókkal bővíthető:

* felhő alapú adatmentés
* webes adatmegjelenítés
* további meteorológiai szenzorok
* energiatakarékos működés (deep sleep)

---

## 📚 Dokumentáció

A projekthez tartozó részletes dokumentáció a `docs` mappában található, amely tartalmazza:

* rendszertervet
* hardver felépítést
* fejlesztési leírást

---

## 👥 Készítők

Egyetemi projekt keretében készült.

---

## 📜 Licenc

Ez a projekt oktatási célból készült.
