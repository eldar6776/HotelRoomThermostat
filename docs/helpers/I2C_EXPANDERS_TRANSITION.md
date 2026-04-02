# I2C GPIO Expander — Tranzicioni Plan

**Datum:** 2026-04-02  
**Status:** Definitivni pinout usvojen  
**Cilj:** Migracija releja i digitalnih I/O sa ESP32-S3 GPIO na I2C ekspander radi eliminacije boot-time glitch-a i oslobađanja GPIO resursa

---

## Definitivni ESP32-S3 Pinout

| ESP32 Pin | Funkcija | Tip | Napomena |
|-----------|----------|-----|----------|
| **IO1**   | NTC 100k B3950 termistor | ADC1_CH0 | Nema promjene u odnosu na prethodnu verziju |
| **IO2**   | I2C1 SDA → Ekspander | Digital I/O | Oslobađa se sa Relay 2 |
| **IO40**  | I2C1 SCL → Ekspander | Digital I/O | Oslobađa se sa Relay 1 |
| **IO41**  | RS485 DE (THVD1500) | Digital OUT | **Promjena:** ranije Window Sensor, sada RS485 DE |
| **IO42**  | Rezerva | — | Slobodan za buduća proširenja |
| **IO48**  | Rezerva | — | Oslobađa se sa RS485 RTS |

### Šta se mijenja u odnosu na prethodnu verziju

| Komponenta | Prije | Poslije | Razlog |
|------------|-------|---------|--------|
| Relay 1 | IO40 (direktno) | PCA9554 P0 | Eliminacija boot glitch-a |
| Relay 2 | IO2 (direktno) | PCA9554 P1 | Eliminacija boot glitch-a |
| Relay 3 | IO42 (direktno) | PCA9554 P2 | Eliminacija boot glitch-a |
| RS485 DE/RTS | IO48 (direktno) | IO41 (direktno) | Oslobađanje IO48, IO41 je čistiji pin |
| Window Sensor | IO41 (direktno) | PCA9554 P6 | Oslobađanje IO41 za RS485 |
| I2C magistrala | — | IO2 + IO40 | Novi I2C1 kanal za ekspander |

---

## PCA9554ADH Pinout (Finalna Konfiguracija)

| PCA9554 Pin | Funkcija | Smjer | Pri boot-u | Hardverska zaštita |
|-------------|----------|-------|------------|-------------------|
| **P0** | Relay 1 (Grijanje/Hlađenje) | OUTPUT | LOW (safe) | 10kΩ pull-down na GND |
| **P1** | Relay 2 (Ventilator brzina 2) | OUTPUT | LOW (safe) | 10kΩ pull-down na GND |
| **P2** | Relay 3 (Ventilator brzina 3) | OUTPUT | LOW (safe) | 10kΩ pull-down na GND |
| **P3** | Rezerva (LED / Buzzer / Budući) | OUTPUT | LOW (safe) | — |
| **P4** | Rezerva (Digitalni input) | INPUT | High-Z | — |
| **P5** | Rezerva (Digitalni input) | INPUT | High-Z | — |
| **P6** | Window Sensor (Active LOW) | INPUT | High-Z | 10kΩ pull-up na 3.3V |
| **P7** | Rezerva (Digitalni input) | INPUT | High-Z | — |

### Adresiranje

```
A0 = GND  → 0
A1 = GND  → 0
A2 = GND  → 0
I2C adresa: 0x20
```

Ako se u budućnosti doda drugi ekspander, adresne pinove ostaviti dostupne za lemljenje.

---

## Sekcija 1 — Testna Demo Faza (PCF8574AN)

### ⚠️ UPOZORENJE: PCF8574AN NIJE GLITCH-FREE

PCF8574AN se koristi **isključivo za testiranje koda i logike**. Pri power-on:

```
PCF8574AN power-on stanje:
├── Svi pinovi → INPUT (high-Z) sa internim pull-up (~100kΩ)
├── Output latch → 0xFF (svi HIGH)
└── ⚠️ Pinovi će čitati HIGH dok se ne konfiguriraju
```

**Rizik:** Releji mogu nakratko okinuti pri paljenju (10-50ms) dok I2C inicijalizacija ne postavi pinove na LOW.

### Hardverske mjere za PCF8574AN testnu fazu

Obavezno dodati hardversku zaštitu na svaki relejni izlaz:

```
PCF8574AN P0 ────[1kΩ]───┬─── Gate MOSFET / Relay drajver
                          │
                         [10kΩ]
                          │
                         GND  (pull-down)
```

Ovo osigurava da relej ostaje OFF čak i ako PCF8574AN pin "lebdi" HIGH pri boot-u.

### PCF8574AN — Inicijalizacija kod

```cpp
#include <Wire.h>

#define PCF8574_ADDR      0x39  // PCF8574AN adresa (A0=A1=A2=GND)
#define PCF8574_REG       0x00  // PCF8574 ima samo jedan register

// I2C1 na IO2 (SDA) + IO40 (SCL)
TwoWire extI2C = TwoWire(1);

// Pin mapiranje (PCF8574: 0=LOW aktivira relej, 1=HIGH deaktivira)
// PCF8574 koristi open-drain izlaze sa internim pull-up
#define RELAY1_BIT  0x01  // P0
#define RELAY2_BIT  0x02  // P1
#define RELAY3_BIT  0x04  // P2
#define WINDOW_BIT  0x40  // P6 (input)

static uint8_t s_pcf_state = 0xFF;  // Početno stanje: svi HIGH (releji OFF)

void pcf8574_init(void)
{
    extI2C.begin(2 /* SDA=IO2 */, 40 /* SCL=IO40 */, 400000);

    // Odmah postavi sve izlaze na HIGH (releji OFF)
    // PCF8574: HIGH = open-drain OFF, LOW = aktiviran
    extI2C.beginTransmission(PCF8574_ADDR);
    extI2C.write(0xFF);
    extI2C.endTransmission();

    s_pcf_state = 0xFF;
}

void pcf8574_set_relay(uint8_t relay_id, bool on)
{
    uint8_t bit = (1 << (relay_id - 1));  // relay 1→bit0, 2→bit1, 3→bit2

    if (on) {
        s_pcf_state &= ~bit;  // LOW = relej ON (open-drain)
    } else {
        s_pcf_state |= bit;   // HIGH = relej OFF
    }

    extI2C.beginTransmission(PCF8574_ADDR);
    extI2C.write(s_pcf_state);
    extI2C.endTransmission();
}

uint8_t pcf8574_read_inputs(void)
{
    extI2C.requestFrom(PCF8574_ADDR, 1);
    if (extI2C.available()) {
        return extI2C.read();
    }
    return 0xFF;  // Greška — vrati sve HIGH
}

bool pcf8574_is_window_open(void)
{
    uint8_t inputs = pcf8574_read_inputs();
    // Window sensor: Active LOW, P6
    return (inputs & WINDOW_BIT) == 0;
}
```

### PCF8574AN — Ključne razlike u odnosu na PCA9554

| Karakteristika | PCF8574AN | PCA9554ADH |
|----------------|-----------|------------|
| **Tip izlaza** | Quasi-bidirekcionalni (open-drain + weak pull-up) | Pravi push-pull output |
| **Power-on stanje** | Pinovi su INPUT sa pull-up HIGH | Pinovi su INPUT (high-Z), output latch = 0x00 |
| **Glitch pri boot-u** | ⚠️ DA — pinovi čitaju HIGH | ✅ NE — pinovi su high-Z |
| **Register struktura** | 1 register (read/write) | 4 registra (Input, Output, Polarity, Config) |
| **I2C transakcija** | Read mijenja smjer pinova | Read/Write su nezavisni |
| **Max I2C brzina** | 100 kHz (Standard mode) | 400 kHz (Fast mode) |
| **Cijena** | ~$0.30 | ~$0.60 |

### PCF8574AN — Ograničenja testne faze

1. **Brzina**: Max 100 kHz I2C. Za releje je dovoljno, ali ne koristiti za brze aplikacije.
2. **Glitch**: Obavezni pull-down otpornici na PCB-u za svaki relejni izlaz.
3. **Read/Write konflikt**: PCF8574 ne razlikuje input/output registre. Čitanje mijenja smjer svih pinova na input na trenutak.
4. **Struja**: Quasi-bidirekcionalni pinovi mogu dati samo ~4mA source, ~20mA sink. Dovoljno za MOSFET gate, ali ne za direktno pokretanje releja.

---

## Sekcija 2 — Finalna Faza (PCA9554ADH)

### ✅ PCA9554ADH — Glitch-Free Garancija

```
PCA9554ADH power-on stanje (hardverski garantovano):
├── Svi pinovi → INPUT (high-Z)
├── Output Port Register → 0x00
├── Configuration Register → 0xFF (svi input)
└── NEMA izlaznog signala dok firmware ne konfigurira
```

### Hardverska shema — PCA9554ADH

```
                    ┌──────────────────┐
   3.3V ───[4.7k]───┤ SDA (IO2)        │
   3.3V ───[4.7k]───┤ SCL (IO40)       │
                    │                  │
   GND ─────────────┤ A0               │
   GND ─────────────┤ A1          P0 ──┼──[1k]──┬── Relay 1 drajver
   GND ─────────────┤ A2          P1 ──┼──[1k]──┼── Relay 2 drajver
                    │             P2 ──┼──[1k]──┼── Relay 3 drajver
   3.3V ───[100nF]──┤ VDD         P3 ──┼──[1k]──┼── Rezerva output
   GND ─────────────┤ GND         P4 ──┼──      ├── Rezerva input
                    │             P5 ──┼──      ├── Rezerva input
                    │             P6 ──┼──[10k]─┤── Window Sensor (pull-up 3.3V)
                    │             P7 ──┼──      ├── Rezerva input
                    │                  │
                    │   [10k]         │
   GND ─────────────┤───┘ (P0-P3)    │
                    └──────────────────┘

Napomena: 10kΩ pull-down na P0-P3 je backup zaštita
          1kΩ serija štiti PCA9554 od povratnih struja
```

### PCA9554ADH — Register mapa

| Register | Adresa | R/W | Opis |
|----------|--------|-----|------|
| Input Port | 0x00 | Read | Čita stanje pinova |
| Output Port | 0x01 | Write | Postavlja izlazne vrijednosti |
| Polarity Inversion | 0x02 | R/W | Invertuje logiku (0=normal, 1=invert) |
| Configuration | 0x03 | R/W | Smjer pinova (0=output, 1=input) |

### PCA9554ADH — Inicijalizacija kod

```cpp
#include <Wire.h>

#define PCA9554_ADDR        0x20  // A0=A1=A2=GND
#define PCA_REG_INPUT       0x00
#define PCA_REG_OUTPUT      0x01
#define PCA_REG_POLARITY    0x02
#define PCA_REG_CONFIG      0x03

// I2C1 na IO2 (SDA) + IO40 (SCL)
TwoWire extI2C = TwoWire(1);

// Pin mapiranje
#define RELAY1_BIT      0x01  // P0
#define RELAY2_BIT      0x02  // P1
#define RELAY3_BIT      0x04  // P2
#define RESERVE_OUT_BIT 0x08  // P3
#define WINDOW_BIT      0x40  // P6 (input)

// Konfiguracija: P0-P3=OUTPUT(0), P4-P7=INPUT(1)
#define PCA_CONFIG      0xF0  // 0b11110000

static uint8_t s_pca_output_state = 0x00;  // Svi izlazi LOW (releji OFF)

void pca9554_init(void)
{
    extI2C.begin(2 /* SDA=IO2 */, 40 /* SCL=IO40 */, 400000);

    // ── GLITCH-FREE INICIJALIZACIJA ──
    // KORAK 1: Postavi output latch na 0 (svi LOW)
    // Pinovi su još uvijek INPUT (high-Z), tako da nema signala na vani
    extI2C.beginTransmission(PCA9554_ADDR);
    extI2C.write(PCA_REG_OUTPUT);
    extI2C.write(0x00);
    extI2C.endTransmission();

    // KORAK 2: Polarity Inversion = 0 (normalna logika)
    extI2C.beginTransmission(PCA9554_ADDR);
    extI2C.write(PCA_REG_POLARITY);
    extI2C.write(0x00);
    extI2C.endTransmission();

    // KORAK 3: Konfiguriraj smjer pinova
    // Tek sada pinovi postaju OUTPUT — ali sa već postavljenom vrijednošću 0
    extI2C.beginTransmission(PCA9554_ADDR);
    extI2C.write(PCA_REG_CONFIG);
    extI2C.write(PCA_CONFIG);  // 0xF0 = P0-P3 output, P4-P7 input
    extI2C.endTransmission();

    s_pca_output_state = 0x00;
}

void pca9554_set_relay(uint8_t relay_id, bool on)
{
    uint8_t bit = (1 << (relay_id - 1));

    if (on) {
        s_pca_output_state |= bit;   // HIGH = relej ON
    } else {
        s_pca_output_state &= ~bit;  // LOW = relej OFF
    }

    extI2C.beginTransmission(PCA9554_ADDR);
    extI2C.write(PCA_REG_OUTPUT);
    extI2C.write(s_pca_output_state);
    extI2C.endTransmission();
}

void pca9554_set_all_relays_off(void)
{
    s_pca_output_state &= ~0x07;  // Clear bitovi 0,1,2 (releji 1-3)
    extI2C.beginTransmission(PCA9554_ADDR);
    extI2C.write(PCA_REG_OUTPUT);
    extI2C.write(s_pca_output_state);
    extI2C.endTransmission();
}

uint8_t pca9554_read_inputs(void)
{
    extI2C.beginTransmission(PCA9554_ADDR);
    extI2C.write(PCA_REG_INPUT);
    extI2C.endTransmission(false);  // Repeated start
    extI2C.requestFrom(PCA9554_ADDR, 1);
    if (extI2C.available()) {
        return extI2C.read();
    }
    return 0x00;
}

bool pca9554_is_window_open(void)
{
    uint8_t inputs = pca9554_read_inputs();
    // Window sensor: Active LOW na P6
    return (inputs & WINDOW_BIT) == 0;
}
```

### PCA9554ADH — API kompatibilnost sa hal.h

Zamijeniti postojeće `digitalWrite()` i `digitalRead()` pozive za releje i window sensor sa PCA9554 funkcijama:

```cpp
// PRIJE (direktni GPIO):
// digitalWrite(PIN_RELAY1, HIGH);
// bool window = (digitalRead(PIN_WINDOW_SENSOR) == LOW);

// POSLIJE (PCA9554):
// pca9554_set_relay(1, true);    // Relay 1 ON
// bool window = pca9554_is_window_open();
```

Preporučuje se kreiranje adapter sloja u `hal.h` koji apstrahuje da li se koristi direktni GPIO ili I2C ekspander:

```cpp
// hal.h — adapter API
void hal_relay_set(uint8_t relay_id, bool on);   // relay_id: 1, 2, 3
bool hal_window_is_open(void);
```

---

## Tranzicioni Plan — Korak po Korak

### Faza 1: PCF8574AN Test (Trenutno)

1. [ ] Spojiti PCF8574AN na breadboard / test PCB
2. [ ] I2C pull-up otpornici: 4.7kΩ na 3.3V za SDA i SCL
3. [ ] **Obavezno:** 10kΩ pull-down na P0, P1, P2 (relejni izlazi)
4. [ ] 1kΩ serija između PCF8574AN pinova i MOSFET gate-ova
5. [ ] Adresni pinovi A0, A1, A2 → GND (adresa 0x39 za PCF8574AN)
6. [ ] Implementirati `pcf8574_init()` i test funkcije
7. [ ] Testirati sve releje, window sensor, RS485 komunikaciju
8. [ ] Validirati Modbus register mapu sa novim relejnim API-jem
9. [ ] Izmjeriti boot vrijeme i eventualne glitch-ove (osciloskop)

### Faza 2: PCA9554ADH Produkcija

1. [ ] Naručiti PCA9554ADH (TSSOP-16 paket)
2. [ ] Dizajnirati footprint na PCB-u
3. [ ] Dodati 100nF decoupling kondenzator na VDD/GND
4. [ ] I2C pull-up: 4.7kΩ na 3.3V
5. [ ] 10kΩ pull-down na P0-P3 (relejni izlazi) — backup zaštita
6. [ ] 1kΩ serija na P0-P3 (zaštita od povratnih struja)
7. [ ] 10kΩ pull-up na P6 (window sensor)
8. [ ] Adresni pinovi A0-A2 → GND (adresa 0x20)
9. [ ] Zamijeniti PCF8574 kod sa PCA9554 kodom (API ostaje isti)
10. [ ] Potvrditi glitch-free boot sa osciloskopom
11. [ ] Finalna validacija svih funkcija

---

## Promjene u Postojećem Kodu

### hal.h — Nove definicije

```c
// I2C Expander pinovi
#define PIN_I2C_SDA         2    // IO2  - I2C1 SDA → Ekspander
#define PIN_I2C_SCL         40   // IO40 - I2C1 SCL → Ekspander

// RS485 — promijenjen pin
#define PIN_RS485_DE        41   // IO41 - THVD1500 Driver Enable

// NTC — ostaje isto
#define PIN_NTC             1    // IO1  - NTC 100k B3950 (ADC1_CH0)

// LCD Backlight — ostaje isto
#define PIN_LCD_BL          38   // IO38 - LCD Backlight (PWM)

// Touch — ostaje isto
#define PIN_TOUCH_SDA       19   // IO19 - GT911 SDA (TP_SDA)
#define PIN_TOUCH_SCL       45   // IO45 - GT911 SCL (TP_SCL)

// ★ RELEJI I WINDOW SENSOR SE VIŠE NE DEFINIŠU KAO DIREKTNI GPIO PINOVI ★
// Oni se sada kontrolišu preko I2C expandera (PCA9554ADH)
```

### hal.cpp — Inicijalizacija

```cpp
// Umjesto direktnih pinMode() za releje:
// pinMode(PIN_RELAY1, OUTPUT); digitalWrite(PIN_RELAY1, LOW);
// pinMode(PIN_RELAY2, OUTPUT); digitalWrite(PIN_RELAY2, LOW);
// pinMode(PIN_RELAY3, OUTPUT); digitalWrite(PIN_RELAY3, LOW);
// pinMode(PIN_WINDOW_SENSOR, INPUT_PULLUP);

// Koristiti:
pca9554_init();  // Ili pcf8574_init() za testnu fazu
```

### hvac.cpp — Zamjena relejnih poziva

```cpp
// PRIJE:
// digitalWrite(PIN_RELAY1, HIGH);
// digitalWrite(PIN_RELAY2, LOW);
// digitalRead(PIN_WINDOW_SENSOR)

// POSLIJE:
// pca9554_set_relay(1, true);
// pca9554_set_relay(2, false);
// pca9554_is_window_open()
```

### modbus_handler.cpp — RS485 DE pin

```cpp
// PRIJE:
// s_mb.begin(&Serial, PIN_RS485_RTS);  // IO48

// POSLIJE:
// s_mb.begin(&Serial, PIN_RS485_DE);   // IO41
```

---

## Bill of Materials — Finalna Konfiguracija

| Komponenta | Količina | Vrijednost | Napomena |
|------------|----------|------------|----------|
| PCA9554ADH | 1 | — | TSSOP-16, I2C GPIO expander |
| Pull-up SDA | 1 | 4.7kΩ 0402 | I2C1 SDA na 3.3V |
| Pull-up SCL | 1 | 4.7kΩ 0402 | I2C1 SCL na 3.3V |
| Pull-down P0 | 1 | 10kΩ 0402 | Relay 1 backup |
| Pull-down P1 | 1 | 10kΩ 0402 | Relay 2 backup |
| Pull-down P2 | 1 | 10kΩ 0402 | Relay 3 backup |
| Pull-down P3 | 1 | 10kΩ 0402 | Rezerva output backup |
| Serija P0 | 1 | 1kΩ 0402 | Relay 1 zaštita |
| Serija P1 | 1 | 1kΩ 0402 | Relay 2 zaštita |
| Serija P2 | 1 | 1kΩ 0402 | Relay 3 zaštita |
| Serija P3 | 1 | 1kΩ 0402 | Rezerva output zaštita |
| Pull-up P6 | 1 | 10kΩ 0402 | Window sensor |
| Decoupling | 1 | 100nF 0402 | PCA9554 VDD/GND |

---

## Napomene

1. **PCF8574AN adresa**: PCF8574AN (sa sufiksom "AN") koristi adresni raspon 0x38-0x3F. PCF8574T (bez "N") koristi 0x20-0x27. Provjeriti oznaku na čipu.
2. **PCA9554ADH paket**: TSSOP-16. Za ručno lemljenje je izvodljiv, ali za proizvodnju preporučiti reflow.
3. **I2C brzina**: PCA9554 podržava do 400 kHz. PCF8574AN je limitiran na 100 kHz.
4. **Napajanje**: Oba čipa rade na 2.3V-5.5V. Koristiti 3.3V za kompatibilnost sa ESP32-S3 GPIO nivoima.
5. **ESD zaštita**: I2C linije na eksternom konektoru bi trebale imati TVS diode (npr. PESD5V0S1UL) za zaštitu od elektrostatskog pražnjenja.
