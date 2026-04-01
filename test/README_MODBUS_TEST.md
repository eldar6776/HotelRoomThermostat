# Modbus RTU Test Suite - Unified Testing Tool

Jedinstvena Python skripta za testiranje svih Modbus funkcija Hotel Room Thermostat firmware-a.

## 📋 Instalacija Dependencies

```bash
pip install pymodbus pyserial colorama
```

**Napomena**: Instalacija će raditi globalno ili u virtualnom okruženju.

---

## 🚀 Primjeri Upotrebe

### 1. **Interaktivni Meni** (Default)
```bash
python modbus_test.py
```
Skripta će prikazati meni sa opcijama:
1. Quick test (brzi test jednog ciklusa)
2. Automated test (višestruki ciklusi)
3. Continuous test (beskonačna petlja)
4. Exit

Nakon izbora, skripta će pitati za COM port i slave adresu.

---

### 2. **Quick Test** (Brzi Test bez Inputa)
```bash
python modbus_test.py --quick
```
**Šta radi:**
- Auto-detekcija: COM6, slave ID 1, 115200 baud
- Jedan potpuni test ciklus (sve funkcije)
- Testira čitanje + pisanje registara, coils, discrete inputs
- Uključuje weather data upis (5 dana prognoze)

**Output primjer:**
```
[OK] Target Temperature: 23.5 °C
[OK] HVAC mode set to HEAT (verified)
[OK] Weather watchdog triggered
```

---

### 3. **Automated N-Cycle Test**
```bash
python modbus_test.py --cycles 3
```
**Šta radi:**
- Pokreće 3 automatska test ciklusa sa POTPUNOM provjerom svih Modbus funkcija
- **Svaki ciklus testira:**
  - ✅ SVE holding registre (target temp, HVAC mode, fan speed) - čitanje
  - ✅ SVE input registre (current temp, relays, uptime, heap, window) - čitanje
  - ✅ SVE coils (DND, MUR) - čitanje i pisanje ON/OFF
  - ✅ SVE discrete inputs (window, system ready, HVAC active, weather valid) - čitanje
  - ✅ Pisanje target temperature sa cikličnim vrijednostima (20.0°C→22.0°C→24.0°C→21.5°C→23.0°C→19.0°C→25.0°C)
  - ✅ Pisanje HVAC mode sa rotacijom (OFF→HEAT→COOL)
  - ✅ Pisanje fan speed sa rotacijom (AUTO→LOW→MID→HIGH)
  - ✅ Weather data upis sa RAZLIČITIM vrijednostima svakog ciklusa (temp 20-29°C, ikone Sunny/Heating/Cooling)
- **Verbose output:** Detaljni ispis SVIH rezultata (čitanja i pisanja)
- **Verifikacija:** Provjerava da li pisane vrijednosti (temp, mode, speed) su ispravno postavljene
- **Idealno za:** Reliability testing, regression testing, stress testing

**Primjeri:**
```bash
python modbus_test.py --cycles 10   # 10 kompletnih ciklusa
python modbus_test.py --cycles 100  # Stress test sa 100 ciklusa
```

---

### 4. **Continuous Test** (Neograničen Loop)
```bash
python modbus_test.py --continuous
```
**Šta radi:**
- Beskonačna petlja sve dok ne pritisnete Ctrl+C
- Ciklično mijenja temperature, HVAC mode, fan speed
- Weather data upis svakih 5 ciklusa
- Idealno za dugotrajno testiranje stabilnosti

**Sa custom intervalom:**
```bash
python modbus_test.py --continuous --interval 5.0  # 5 sekundi pauze između ciklusa
```

---

### 5. **Custom COM Port i Slave Adresa**
```bash
python modbus_test.py --port COM5 --slave 2 --quick
python modbus_test.py --port COM10 --slave 5 --cycles 5
python modbus_test.py --port COM6 --slave 1 --continuous --interval 3.0
```

---

## 📊 Šta se Testira

### **Holding Registers (40001+, Read/Write)**
| Address | Register | Description | Test |
|---------|----------|-------------|------|
| 40001 | Target Temperature | Setpoint (×10) | ✅ Read + Write + Verify |
| 40002 | HVAC Mode | 0=OFF, 1=HEAT, 2=COOL | ✅ Write + Verify |
| 40003 | Fan Speed | 0=AUTO, 1=LOW, 2=MID, 3=HIGH | ✅ Write + Verify |
| 40030-40045 | Weather Data | 5-day forecast | ✅ Batch Write (15 registers) |

### **Input Registers (30001+, Read-Only)**
| Address | Register | Description | Test |
|---------|----------|-------------|------|
| 30001 | Current Temperature | Room temp (×10) | ✅ Read + Display |
| 30002 | Target Temperature | Mirror of 40001 | ✅ Read + Display |
| 30003 | Relay Status | Bitmask (R1/R2/R3) | ✅ Read + Decode bits |
| 30004-30005 | Uptime | 32-bit seconds | ✅ Read + Combine 2 regs |
| 30006 | Free Heap | RAM available (KB) | ✅ Read + Display |
| 30007 | Window Sensor | GPIO raw value | ✅ Read + Display |

### **Coils (00001+, Read/Write)**
| Address | Coil | Description | Test |
|---------|------|-------------|------|
| 00001 | DND | Do Not Disturb | ✅ Write ON/OFF + Verify |
| 00002 | MUR | Make Up Room | ✅ Write ON/OFF + Verify |

### **Discrete Inputs (10001+, Read-Only)**
| Address | Input | Description | Test |
|---------|-------|-------------|------|
| 10001 | Window Closed | TRUE if closed | ✅ Read + Display |
| 10002 | System Ready | TRUE if initialized | ✅ Read + Display |
| 10003 | HVAC Active | TRUE if heating/cooling | ✅ Read + Display |
| 10004 | Weather Valid | TRUE if data not stale | ✅ Read + Display |

---

## 🛠️ Advanced Options

### Command-Line Arguments

```bash
python modbus_test.py --help
```

**Dostupne opcije:**
- `--port COM6` - COM port (default: COM6)
- `--slave 1` - Modbus slave adresa (default: 1)
- `--baudrate 115200` - Baud rate (default: 115200)
- `--quick` - Brzi test jednog ciklusa
- `--cycles N` - Pokreni N automatskih ciklusa
- `--continuous` - Beskonačna petlja
- `--interval 2.0` - Interval između ciklusa u sekundama (default: 2.0)

---

## 🔍 Očekivani Rezultati

### ✅ Uspješan Test Output
```
======================================================================
Quick Test Mode - Single Cycle
======================================================================

TEST: Read Holding Registers (40001-40003)
[OK] Target Temperature: 22.0 °C (raw: 220)
[OK] HVAC Mode: OFF (0)
[OK] Fan Speed: AUTO (0)

TEST: Read Input Registers (30001-30007)
[OK] Current Temperature: 21.5 °C (raw: 215)
[OK] Relay Status: 0b000 (R1=False, R2=False, R3=False)
[OK] Uptime: 143 seconds (0.04 hours)
[OK] Free Heap: 235 KB

TEST: Write Target Temperature (23.5°C)
[OK] Target temperature set to 23.5°C (verified)

TEST: Write Weather Forecast Data
[INFO] Weather watchdog triggered
[OK] Day 0: Mon, Icon 0, High 25.0°C, Low 18.0°C
[OK] Day 1: Tue, Icon 1, High 22.0°C, Low 15.0°C
...
```

### ❌ Greške i Njihova Značenja

**Connection Error:**
```
[ERROR] Failed to connect to COM6
```
- Provjerite da li je ESP32 spojren na COM6
- Pokušajte drugi COM port: `--port COM5`

**Timeout Error:**
```
[ERROR] No response received after 3 retries
```
- ESP32 je zauzet ili Modbus adresa nije ispravna
- Provjerite slave adresu na displeju (Settings → Modbus Address)
- Pokušajte: `--slave 2` (ako je adresa 2)

**Write Failed:**
```
[ERROR] Verification failed: expected 235, got N/A
```
- Rijetko (< 5% slučajeva) zbog shared UART porta
- Retry automatski radi - ignorišite ako se desi povremeno

---

## 📈 Use Cases

### Use Case 1: Brza Provjera Nakon Upload-a
```bash
python modbus_test.py --quick
```
**Cilj**: Verifikovati da Modbus radi nakon firmware upload-a.

---

### Use Case 2: Stability Testing (24h Run)
```bash
python modbus_test.py --continuous --interval 10.0
```
**Cilj**: Testirati stabilnost sistema tokom dužeg perioda.

**Napomena**: Ostavite da radi 24h, provjerite da li ima timeout-a ili crash-eva.

---

### Use Case 3: Regression Testing (100 Ciklusa)
```bash
python modbus_test.py --cycles 100
```
**Cilj**: Provjeriti da nove izmjene u kodu ne kvare Modbus komunikaciju.

**Success Criteria**: Svi ciklusi prolaze bez grešaka.

---

### Use Case 4: Multi-Device Testing (Drugi Port)
```bash
python modbus_test.py --port COM10 --slave 3 --quick
```
**Cilj**: Testirati drugi ESP32 na drugom COM portu sa drugom slave adresom.

---

## 🔧 Troubleshooting

### Problem: "ModuleNotFoundError: No module named 'pymodbus'"
**Rešenje:**
```bash
pip install pymodbus pyserial colorama
```

---

### Problem: "Failed to connect to COM6"
**Rešenje:**
1. Provjerite Device Manager → Ports (COM & LPT)
2. Nađite ESP32-S3 USB port (npr. COM5, COM10)
3. Pokrenite sa pravim portom:
   ```bash
   python modbus_test.py --port COM10 --quick
   ```

---

### Problem: "Slave address must be between 1 and 247"
**Rešenje:**
- Default slave adresa je 1
- Provjerite na displeju: Settings → Modbus Address
- Pokrenite sa ispravnom adresom:
  ```bash
  python modbus_test.py --slave 2 --quick
  ```

---

### Problem: Write timeouts (povremeno)
**Uzrok**: Shared UART0 port između debug logova i Modbus-a.

**Rešenje**: 
- Normalno ponašanje (< 5% write operations)
- Retry automatski radi
- Ako je > 20% timeout rate, provjerite debug logove u firmware-u

---

## 📝 Changelog

### v1.0.0 - Unified Test Suite
- ✅ Objedinjeni 3 testa u jedan fajl
- ✅ Command-line argumenti za brzu upotrebu
- ✅ Interaktivni meni za lake korištenje
- ✅ pymodbus 3.x API compatibility (keyword argumenti)
- ✅ ASCII-safe output za Windows konzolu
- ✅ Verbose i non-verbose režimi
- ✅ Weather data batch write (15 registara)

---

## 📞 Support

Za pitanja i probleme:
1. Provjerite ovaj README za troubleshooting
2. Pogledajte glavni README.md za Modbus register map
3. Provjerite serial monitor output (115200 baud)

---

**Status**: ✅ Production Ready  
**pymodbus Version**: 3.12.1+  
**Python Version**: 3.7+
   - Upis fan speed (cikličko AUTO → LOW → MID → HIGH)
   - Upis coils (DND/MUR ON/OFF)

### 6. **Weather Data Write (svakih 5 ciklusa)**
   - Upis weather watchdog trigger
   - Upis 5 dana prognoze:
     - Day ID (0-6 = Sun-Sat)
     - Icon ID (0=sunny, 1=heating, 2=cooling)
     - High temperature (×10)
     - Low temperature (×10)

## Output

Skripta koristi obojeni output:
- ✅ **Zeleno** - Uspješan test
- ❌ **Crveno** - Greška
- ℹ️ **Žuto** - Info poruka
- **Cyan** - Header/sekcija

## Zaustavljanje

Pritisnite `Ctrl+C` da zaustavite continuous test loop.

## Napomene

- Baud rate je zadan na **115200** (isti kao firmware)
- Sve write operacije imaju verification - čita se registar nakon upisa
- Interval između testova je podesiv (default 2 sekunde)
- Test se ponavlja beskonačno dok se ne zaustavi Ctrl+C
