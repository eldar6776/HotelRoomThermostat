# SISTEMSKI DIJAGRAMI (Mermaid)
Ovi dijagrami vizualizuju arhitekturu i logiku definisanu u FSD.md (v4.0 — I2C Expander).

---

## 1. Arhitektura Sistema (Block Diagram)
Prikazuje vezu između ESP32-S3, I2C GPIO Expandera, Custom PCB-a i vanjskih elemenata.

```mermaid
graph TD
    subgraph "ESP32-S3 (4848S040)"
        MCU[ESP32-S3 MCU]
        LCD[ST7701 4.0 LCD]
        Touch[GT911 Touch]
        NVS[(NVS Flash)]
    end

    subgraph "I2C0 Bus (Wire)"
        I2C0_SDA[IO19 SDA]
        I2C0_SCL[IO45 SCL]
    end

    subgraph "I2C1 Bus TwoWire1"
        I2C1_SDA[IO2 SDA]
        I2C1_SCL[IO40 SCL]
    end

    subgraph "I2C GPIO Expander"
        EXP_P0[P0 - Relay 1]
        EXP_P1[P1 - Relay 2]
        EXP_P2[P2 - Relay 3]
        EXP_P3[P3 - Rezerva Out]
        EXP_P4[P4 - Rezerva Out]
        EXP_P5[P5 - Rezerva Out]
        EXP_P6[P6 - Window Sensor]
        EXP_P7[P7 - Rezerva In]
    end

    subgraph "Custom PCB Interfejs"
        RS485[RS485 THVD1500]
        ADC[NTC 100k B3950]
    end

    MCU -- "RGB 16-bit parallel" --> LCD
    MCU -- "I2C0 IO19/IO45" --> Touch
    MCU -- "I2C1 IO2/IO40 400kHz" --> I2C1_SDA
    I2C1_SDA --> EXP_P0
    I2C1_SCL --> EXP_P0
    MCU -- "UART0 + DE IO41" --> RS485
    MCU -- "PWM IO38" --> LCD
    MCU -- "ADC1_CH0 IO1" --> ADC

    RS485 <== "Modbus RTU 115200" ==> Master[Zidni Kontroler / Server]
    EXP_P0 --> RELAY1[Relay 1 Heat/Cool]
    EXP_P1 --> RELAY2[Relay 2 Fan Speed 2]
    EXP_P2 --> RELAY3[Relay 3 Fan Speed 3]
    RELAY1 --> HVAC[Grijanje/Hlađenje/Fan]
    RELAY2 --> HVAC
    RELAY3 --> HVAC
    EXP_P6 --> Window[Senzor Prozora Active LOW]
    ADC --> Temp[Senzor Temperature]
```

---

## 2. Dual I2C Bus Arhitektura
Prikazuje nezavisne I2C0 i I2C1 kontrolere na ESP32-S3.

```mermaid
graph LR
    subgraph "ESP32-S3"
        I2C0_HW[I2C0 Hardverski Kontroler]
        I2C1_HW[I2C1 Hardverski Kontroler]
    end

    subgraph "I2C0 Bus 100kHz"
        I2C0_SDA[IO19 SDA]
        I2C0_SCL[IO45 SCL]
        TOUCH[GT911 Touch 0x5D]
    end

    subgraph "I2C1 Bus 400kHz"
        I2C1_SDA[IO2 SDA]
        I2C1_SCL[IO40 SCL]
        EXPANDER[GPIO Expander 0x38/0x20]
    end

    I2C0_HW --> I2C0_SDA
    I2C0_HW --> I2C0_SCL
    I2C0_SDA --> TOUCH
    I2C0_SCL --> TOUCH

    I2C1_HW --> I2C1_SDA
    I2C1_HW --> I2C1_SCL
    I2C1_SDA --> EXPANDER
    I2C1_SCL --> EXPANDER

    style I2C0_HW fill:#e1f5fe
    style I2C1_HW fill:#f3e5f5
    style TOUCH fill:#e8f5e9
    style EXPANDER fill:#fff3e0
```

---

## 3. Navigacija Glavnog Interfejsa (Beskonačna Petlja / Nezavisni Ekrani)
Glavni interfejs koristi tri nezavisna ekrana povezana navigacionim dugmadima (bez SwipeContainer-a).

```mermaid
stateDiagram-v2
    [*] --> Screen_Main

    Screen_Main --> Screen_Thermostat : Next Btn
    Screen_Thermostat --> Screen_Main : Prev Btn
    
    Screen_Thermostat --> Screen_Clean : Next Btn
    Screen_Clean --> Screen_Thermostat : Prev Btn
    
    Screen_Clean --> Screen_Main : Next Btn

    state Screen_Main {
        [*] --> Home_View
        Home_View --> Hidden_Menu_Trigger : Long Press (5s)
    }

    Hidden_Menu_Trigger --> PIN_Entry : Switch Screen
    PIN_Entry --> Settings_1 : PIN = 1234
    PIN_Entry --> Screen_Main : PIN != 1234 / Timeout
```

---

## 4. Kružna Navigacija Postavki (Settings Menu)
Prikazuje kretanje između tri ekrana postavki sa Save/Exit logikom.

```mermaid
flowchart LR
    S1[Settings 1] -->|BtnNext1| S2[Settings 2]
    S2 -->|BtnNext2| S3[Settings 3]
    S3 -->|BtnNext3| S1

    S1 -->|BtnSave1| SaveAction[Save & Exit to Main]
    S2 -->|BtnSave2| SaveAction
    S3 -->|BtnSave3| SaveAction

    subgraph "Save Action Logic"
        SaveAction --> CheckFlags{Dirty Flags?}
        CheckFlags -- "!= 0" --> NVS_Write[Update NVS]
        NVS_Write --> Exit[Load ui_Main]
        CheckFlags -- "== 0" --> Exit
    end
```

---

## 5. Inactivity Timeout i WiFi Manager Logic
Prikazuje kako WiFi Manager suspenduje automatski povratak na glavni ekran.

```mermaid
stateDiagram-v2
    [*] --> Settings_Active

    state Settings_Active {
        [*] --> Timer_Running
        Timer_Running --> Timer_Reset : LV_EVENT_PRESSED
        Timer_Running --> Exit_To_Main : 30s Timeout
    }

    Settings_Active --> WiFi_AP_Mode : Toggle Switch (ON)

    state WiFi_AP_Mode {
        [*] --> Timer_Paused
        Timer_Paused --> AP_Active
    }

    WiFi_AP_Mode --> Settings_Active : Toggle Switch (OFF)
```

---

## 6. Weather UI Update Flow
Ažuriranje 5 statičkih kartica na osnovu novih podataka.

```mermaid
flowchart TD
    Update[Modbus Write / API Trigger] --> Loop[Za svaki dan 1-5]
    Loop --> Map[Mapiranje CardDay i Labela]
    Map --> SetDay[lv_label_set_text ui_LabelDayX]
    SetDay --> SetHigh[lv_label_set_text ui_LabelTempHighX]
    SetHigh --> SetLow[lv_label_set_text ui_LabelTempLowX]
    SetLow --> SetIcon[lv_img_set_src ui_ImgIconX]
    SetIcon --> Next{Svi dani?}
    Next -- "NE" --> Loop
    Next -- "DA" --> End[Kraj Osvježavanja]
```

---

## 7. Boot Sekvenca sa I2C Expanderom
Timeline od power-on-a do potpuno funkcionalnog sistema.

```mermaid
sequenceDiagram
    participant PWR as Power-On
    participant ROM as ROM Bootloader
    participant BL as 2nd Stage Bootloader
    participant FW as Arduino Framework
    participant HAL as board_hal_init()
    participant EXP as I2C Expander
    participant RS485 as RS485 DE (IO41)
    participant DISP as Display (ST7701)
    participant LVGL as LVGL + Touch
    participant MB as Modbus + HVAC

    PWR->>ROM: 0ms
    ROM->>BL: 50ms (strapping pins: 0,3,45,46)
    BL->>FW: 200ms
    FW->>HAL: 400ms (setup())
    HAL->>EXP: 400-410ms I2C1 init (IO2/IO40)
    EXP-->>HAL: 410-420ms Releji = OFF (safe state)
    HAL->>RS485: 420ms DE = LOW (receive mode)
    HAL->>DISP: 430-550ms gfx->begin()
    HAL->>LVGL: 550ms lv_init() + Touch
    LVGL->>MB: 600ms Modbus + HVAC init
    MB-->>FW: System Ready
```

---

## 8. I2C Expander Adapter API — Pozivni Lanac
Kako `hvac.cpp` i `modbus_handler.cpp` komuniciraju sa relejima kroz adapter sloj.

```mermaid
graph TD
    HVAC[hvac.cpp] --> HAL_RELAY[hal_relay_set / hal_relay_is_on]
    MODBUS[modbus_handler.cpp] --> HAL_RELAY
    HAL_RELAY --> TYPE_CHECK{EXPANDER_TYPE?}

    TYPE_CHECK -- "PCF8574AN" --> PCF[pcf8574_write / pcf8574_read]
    TYPE_CHECK -- "PCA9554ADH" --> PCA[pca9554_write_output / pca9554_read_inputs]

    PCF --> I2C1[I2C1 Bus IO2/IO40]
    PCA --> I2C1
    I2C1 --> EXP_HW[GPIO Expander Hardware]

    EXP_HW --> R1[Relay 1 P0]
    EXP_HW --> R2[Relay 2 P1]
    EXP_HW --> R3[Relay 3 P2]
    EXP_HW --> WIN[Window Sensor P6]

    style HAL_RELAY fill:#e1f5fe
    style TYPE_CHECK fill:#fff3e0
    style I2C1 fill:#e8f5e9
```
