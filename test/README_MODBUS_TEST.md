# Modbus RTU Test Script

Python skripta za testiranje svih Modbus funkcija Hotel Room Thermostat firmware-a.

## Instalacija Dependencies

```bash
pip install pymodbus pyserial colorama
```

## Upotreba

```bash
cd test
python modbus_test.py
```

Skripta će pitati:
1. **COM port** (npr. `COM6`)
2. **Modbus slave adresu** (1-247, default je 1)
3. **Interval testiranja** (u sekundama, default 2.0)

## Šta se testira

### 1. **Read Holding Registers (40001-40003)**
   - Target temperatura (×10)
   - HVAC mode (OFF/HEAT/COOL)
   - Fan speed (AUTO/LOW/MID/HIGH)

### 2. **Read Input Registers (30001-30007)**
   - Trenutna temperatura (×10)
   - Target temperatura (mirror)
   - Relay status (bit field)
   - Uptime (32-bit sekunde)
   - Free heap (KB)
   - Window sensor raw

### 3. **Read Coils (00001-00002)**
   - DND (Do Not Disturb)
   - MUR (Make Up Room)

### 4. **Read Discrete Inputs (10001-10004)**
   - Window sensor status
   - System ready flag
   - HVAC active flag
   - Weather data valid flag

### 5. **Write Tests**
   - Upis target temperature (cikličko 20°C, 22°C, 24°C, 21.5°C)
   - Upis HVAC mode (cikličko OFF → HEAT → COOL)
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
