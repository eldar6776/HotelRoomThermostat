# HVAC Diagnostics & QA Checklist

Ovaj dokument služi za sistematsku provjeru ispravnosti rada HVAC logike i prevenciju regresija.

## 1. NTC Senzor i Zaštita
- [ ] **Standardno očitavanje:** Prikaz temperature stabilan, bez brzih preskoka (zahvaljujući EMA filteru).
- [ ] **Senzor isključen (Otvoren krug):** Treba prikazati grešku, ne smije ispisati 6500°C (potrebno fiksati provjeru donjeg praga/ADC rail-a).
- [ ] **Senzor kratak spoj:** Treba prikazati grešku, HVAC releji se moraju SIGURNOSNO ugasiti.
- [ ] **Offset Kalibracija:** Promjena offseta u `Settings 2` trenutno utiče na izračun (vidljivo i na displeju i u HVAC logici).

## 2. Ponašanje Ventilatora (AUTO mod)
- [ ] **Boot State:** Nakon potpunog reboota, ventilator glatko prolazi iz HIGH -> MID -> LOW kako se temperatura približava Setpoint-u (više se ne zaglavljuje zahvaljujući `0.2f` fan_diff fix-u).
- [ ] **Modbus Override:** Promjena brzine ventilatora u AUTO preko Modbus registra (40003) trenutno forsira re-kalkulaciju brzine.

## 3. Ponašanje Releja (Deadband & Hardware)
- [ ] **100ms Interlock:** Pri prebacivanju npr. iz Brzine 1 u Brzinu 2, svi releji se prvo gase, a nakon >100ms se pali novi relej (zaštita od kratkog spoja ventilatora).
- [ ] **Mode 1-Relay:** Kada je aktivan Control Type 1 (Single Relay), promjena ventilatora na ekranu nema efekta, samo Relej 1 okida na histerezu.
- [ ] **Glitch-free Boot:** Prilikom paljenja ESP32-S3 (ako se koristi PCA9554ADH), releji NE smiju ni na stotinku milisekunde zatitrati na izlazu.

## 4. Sigurnosni Prekidi
- [ ] **Window Sensor (P6 na expanderu):** Spajanje P6 na GND odmah simulira prozor "Otvoren".
- [ ] Svi releji automatski isključeni u roku od ~1 sec od otvaranja prozora.
- [ ] Displej obavještava o otvorenom prozoru, zabranjeno paljenje grijanja/hlađenja nazad.
- [ ] Zatvaranje prozora vraća sistem u prethodni režim rada tek uz obavezno čekanje deadbanda.

## 5. Modbus Sinhronizacija
- [ ] Zadavanje modova i brzina preko RS485 ne "bori se" sa korisničkim unosom, Modbus služi kao "Single Source of Truth".
- [ ] Promjena `Settings` vrijednosti na displeju pravovremeno popunjava i ažurira modbus registre pri izlasku.