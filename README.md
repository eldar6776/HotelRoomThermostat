# Hotel Room Thermostat

Kompletan hotelski sobni termostat razvijen kao gotov proizvod za modernu hotelsku sobu. Uređaj objedinjuje elegantan GUI na ESP32-S3 platformi, prilagođeni PCB interfejs za hotelske instalacije i potpunu Modbus/RS485 komunikaciju za integraciju sa nadređenim sistemima.

Ovo nije samo ekran sa termostatom, već potpuno zaokružen uređaj za kontrolu komfora i statusa sobe — sa lokalnim korisničkim interfejsom, hotelskim funkcijama, mrežnim servisnim mogućnostima i hardverskom prilagodbom koja omogućava pouzdan rad u realnoj instalaciji.

## Pregled uređaja

Glavne funkcije uređaja:

- upravljanje grijanjem i hlađenjem
- podešavanje zadate temperature i brzine ventilatora
- prikaz vremena i statusa sistema
- **DND** (*Do Not Disturb*) dugme
- **MUR** (*Make Up Room*) dugme za poziv sobarice
- elegantan GUI prilagođen hotelskom okruženju
- upload logotipa hotela preko web interfejsa
- firmware update preko web interfejsa
- Wi‑Fi povezivanje za servisne i mrežne funkcije
- periodična sinhronizacija vremena preko mreže kada uređaj ne dobija time-sync paket iz hotelskog sistema
- integracija sa hotelskim sistemom preko **RS485 / Modbus** komunikacije

## Galerija GUI interfejsa

<table>
  <tr>
    <td><img src="doc/gui1.jpg" alt="GUI screen 1" width="100%"></td>
    <td><img src="doc/gui2.jpg" alt="GUI screen 2" width="100%"></td>
  </tr>
  <tr>
    <td><img src="doc/gui3.jpg" alt="GUI screen 3" width="100%"></td>
    <td><img src="doc/gui4.jpg" alt="GUI screen 4" width="100%"></td>
  </tr>
</table>

GUI je vrlo ugodan, čist i vizuelno moderan, sa izgledom koji se dobro uklapa u hotelski enterijer. Interfejs djeluje mirno i uredno, ali istovremeno daje sve ključne informacije i komande na prvi pogled, što je posebno važno za uređaj koji koriste i gosti i osoblje.

## Hardverska platforma

Uređaj je baziran na **ESP32-S3 4848S040** platformi sa 4.0" touchscreen displejom i razvijen je zajedno sa posebnim **PCB interfejsom** koji ga pretvara u ozbiljno rješenje za hotelsku sobu.

Prilagođeni interfejs obuhvata:

- **I2C expander** za proširenje I/O funkcionalnosti
- **flicker-free** rješenje prilagođeno stabilnom radu interfejsa i izlaza
- **RS485 driver** za robusnu komunikaciju u objektu
- dodatni hardverski interfejs za povezivanje sobnih funkcija i signalnih linija

Takav pristup omogućava da je osnovni, cjenovno pristupačan ESP display modul pretvoren u mnogo sposobniji i profesionalniji hotelski uređaj.

## Hardverske modifikacije i praktični workaround-i

Zbog prirode jeftinog ESP baziranog displeja korišteno je nekoliko pažljivo izvedenih hardverskih prilagodbi kako bi uređaj dobio funkcionalnost potrebnu za realnu primjenu.

### Mjerenje temperature

Sam display modul nema ugrađen temperaturni senzor, pa je urađen mali hardverski hack koji omogućava mjerenje temperature i korištenje uređaja kao stvarnog sobnog termostata, a ne samo kao komandnog panela.

### Serijski interfejs

Napravljen je i dodatni workaround na serijskoj vezi kako bi paralelno mogli raditi:

- **USB-C programer / servisni pristup**
- **Modbus / RS485 driver**

Na taj način je omogućeno i razvojno programiranje i terenska komunikacija bez odricanja od jedne od te dvije funkcije.

### Fotografije hardverskih izmjena

<table>
  <tr>
    <td><img src="doc/workaround1.jpg" alt="Hardware workaround 1" width="100%"></td>
    <td><img src="doc/workaround2.jpg" alt="Hardware workaround 2" width="100%"></td>
  </tr>
</table>

## Komunikacija i integracija

Komunikacioni sloj je jedna od najjačih strana ovog uređaja.

Implementiran je kompletan **Modbus** sa funkcijama za:

- read operacije
- write operacije
- integraciju sa hotelskim nadzornim i upravljačkim sistemima
- razmjenu statusa, komandi i parametara uređaja

Kombinacija **RS485 drivera** i kompletne Modbus implementacije čini uređaj spremnim za ozbiljnu BMS / hotelsku integraciju.

## Softverske mogućnosti

Firmware objedinjuje nekoliko veoma korisnih funkcija:

- moderan touchscreen GUI baziran na LVGL ekosistemu
- lokalno upravljanje sobnim komforom
- čuvanje podešavanja
- prikaz i obrada hotelskih statusa
- upload prilagođenog logotipa hotela
- web firmware update
- Wi‑Fi konekcija za servis i mrežne funkcije
- periodično osvježavanje vremena preko mreže kada nema sinhronizacije iz hotelskog sistema

To uređaj čini praktičnim i za OEM prilagodbu i za direktnu ugradnju u hotelske projekte.

## Struktura repozitorija

- `fw/` — firmware za ESP32-S3 uređaj
- `hw/` — hardverski dizajn i PCB projekat interfejsne ploče
- `sw/` — GUI / SquareLine Studio projekat i resursi interfejsa
- `doc/` — fotografije GUI-ja, hardverskih prilagodbi i dodatna dokumentacija

## Razvojni stack

- **MCU platforma:** ESP32-S3
- **Firmware framework:** Arduino / PlatformIO
- **GUI:** LVGL + SquareLine Studio
- **Komunikacija:** RS485 + Modbus
- **Storage / assets:** LittleFS i web upload resursa
- **Hardware design:** prilagođeni PCB interfejs

## Zašto je projekat zanimljiv

Ovaj projekat je dobar primjer kako se od povoljnog ESP touchscreen modula može napraviti ozbiljan i vrlo lijep gotov uređaj za hotelsku sobu. Posebno je zanimljiva kombinacija:

- atraktivnog i prijatnog korisničkog interfejsa
- stvarnih hotelskih funkcija kao što su DND i MUR
- prilagođenog PCB-a za profesionalno povezivanje
- kompletne industrijske komunikacije preko RS485 / Modbus
- praktičnih hardverskih workaround-a koji rješavaju ograničenja osnovnog modula

Rezultat je kompaktan, elegantan i funkcionalno zaokružen sobni kontroler spreman za upotrebu u hotelskom okruženju.
