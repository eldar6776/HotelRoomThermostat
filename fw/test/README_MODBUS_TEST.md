# Modbus RTU Test Suite - Unified Testing Tool

Jedinstvena i robusna Python skripta za testiranje svih Modbus funkcija Hotel Room Thermostat firmware-a.

## 📋 Instalacija Dependencies

```bash
pip install pymodbus pyserial colorama
```

---

## 🚀 Primjeri Upotrebe

### 1. **Interaktivni Meni** (Manualna Kontrola)
```bash
python modbus_test.py
```
Odabirom opcije `1` ulazite u **Interactive Control**, gdje možete direktno slati komande:
- Čitanje punog stanja (Senzori, Releji, Fault alarmi, Postavke)
- Podešavanje Target Temperature, HVAC Moda i Fan Speed-a
- Promjena Relay Moda (3-Speed ili 1-Relay)
- Paljenje i gašenje DND i MUR statusa

---

### 2. **Automated N-Cycle Test** (Brzi ili produženi)
```bash
python modbus_test.py --quick        # Pokreće 1 brzi ciklus
python modbus_test.py --cycles 3     # Pokreće 3 automatizirana ciklusa
```
**Šta radi:**
- Čita **sve** inputRegistre (uključujući `Sensor Fault` i dijagnostiku memorije).
- Čita **sve** discreteInputs i coils.
- Prolazi kroz sve kombinacije HVAC modova (OFF, HEAT, COOL).
- Prolazi kroz sve brzine ventilatora (AUTO, LOW, MID, HIGH).
- Mijenja Relay kontrole.

---

### 3. **Continuous Stress Test**
```bash
python modbus_test.py --continuous
```
Pokreće beskonačnu petlju testiranja. Idealno za ostaviti preko noći kako bi testirali stabilnost RS485 komunikacije i termostata pod punim opterećenjem.

---

### 4. **Custom COM Port i Slave Adresa**
```bash
python modbus_test.py --port COM5 --slave 2 --quick
```

---

## 📊 Šta se Testira

### **Holding Registers (40001+, Read/Write)**
| Address | Register | Description |
|---------|----------|-------------|
| 40001 | Target Temperature | Setpoint (×10) |
| 40002 | HVAC Mode | 0=OFF, 1=HEAT, 2=COOL |
| 40003 | Fan Speed | 0=AUTO, 1=LOW, 2=MID, 3=HIGH |
| 40022 | Relay Mode | 0=3-Speed, 1=1-Relay |

### **Input Registers (30001+, Read-Only)**
| Address | Register | Description |
|---------|----------|-------------|
| 30001 | Current Temperature | Room temp (×10) |
| 30002 | Target Temperature | Mirror of 40001 |
| 30003 | Relay Status | Bitmask (R1/R2/R3) |
| 30004-30005 | Uptime | 32-bit seconds |
| 30006 | Free Heap | RAM available (KB) |
| 30007 | Window Sensor | GPIO raw value |
| 30008 | Sensor Fault | **1=Fault**, 0=OK |

### **Coils & Discrete Inputs (00001+ & 10001+)**
- **Coils**: DND (00001) i MUR (00002) - Read/Write
- **Discrete**: Window Closed (10001), System Ready (10002), HVAC Active (10003), **Sensor Fault Alarm (10004)**

---

## 🛡️ Napredne Funkcionalnosti Skripte

- **Auto-Retry Logika**: Ukoliko dođe do smetnje na RS485 kablu, skripta ne puca, već automatski pokušava poslati komandu do 3 puta prije nego prijavi grešku. Ovo uveliko smanjuje lažne uzbune (false-positives).
- **Optimizacija Bandwidth-a**: Skripta čita registre u blokovima (batch read), čime se drastično ubrzava odziv.
- **Color-coded output**: Jasne boje (Zeleno za OK, Crveno za ERROR) omogućuju brzo prepoznavanje stanja u konzoli.

---

## 🔧 Troubleshooting

### Problem: "Failed to connect to COM6"
**Rešenje:**
- Provjerite u *Device Manageru* koji COM port ESP32 koristi (npr. COM10).
- Pokrenite sa pravim portom: `python modbus_test.py --port COM10`

### Problem: "Attempt 3 failed: Modbus exception"
**Rešenje:**
- Provjerite da li se Slave ID poklapa sa onim u meniju termostata (Default: 1).
- Provjerite ožičenje (A i B žice na RS485 modulu).
