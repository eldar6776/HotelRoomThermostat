# SISTEMSKI DIJAGRAMI (Mermaid)
Ovi dijagrami vizualizuju arhitekturu i logiku definisanu u FSD.md (v3.0).

---

## 1. Arhitektura Sistema (Block Diagram)
Prikazuje vezu između ESP32-S3, Custom PCB-a i vanjskih elemenata.

```mermaid
graph TD
    subgraph "ESP32-S3 (4848S040)"
        MCU[ESP32-S3 MCU]
        LCD[ST7701 4.0' LCD]
        Touch[GT911 Touch]
        NVS[(NVS Flash)]
    end

    subgraph "Custom PCB Interfejs"
        RS485[RS485 THVD1500]
        Relays[3x Releji ULN2003]
        ADC[NTC 100k B3950]
        DIN[Senzor Prozora]
    end

    MCU -- "RGB 16-bit" --> LCD
    MCU -- "I2C (IO19/20)" --> Touch
    MCU -- "UART + RTS (IO48)" --> RS485
    MCU -- "PWM (IO38)" --> LCD
    
    RS485 <== "Modbus RTU" ==> Master[Zidni Kontroler / Server]
    Relays -- "IO40, IO2, IO39" --> HVAC[Grijanje/Hlađenje/Fan]
    ADC -- "IO1 (Analog)" --> Temp[Senzor Temperature]
    DIN -- "IO41 (Digital)" --> Window[Senzor Prozora]
```

---

## 2. Navigacija Glavnog Interfejsa (TileView)
Glavni interfejs koristi horizontalni `ui_SwipeContainer`.

```mermaid
stateDiagram-v2
    [*] --> Tile1_Main
    
    Tile1_Main --> Tile2_Thermo : Swipe Right / BtnClick
    Tile2_Thermo --> Tile1_Main : Swipe Left
    
    Tile2_Thermo --> Tile3_Weather : Swipe Right (Watchdog OK)
    Tile3_Weather --> Tile2_Thermo : Swipe Left
    
    Tile3_Weather --> Tile4_Clean : Swipe Right
    Tile4_Clean --> Tile3_Weather : Swipe Left

    state Tile1_Main {
        [*] --> Home_View
        Home_View --> Hidden_Menu_Trigger : Long Press (5s)
    }

    Hidden_Menu_Trigger --> PIN_Entry : Switch Screen
    PIN_Entry --> Settings_1 : PIN = 1234
    PIN_Entry --> Tile1_Main : PIN != 1234 / Timeout
```

---

## 3. Kružna Navigacija Postavki (Settings Menu)
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

## 4. Inactivity Timeout i WiFi Manager Logic
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

## 5. Weather UI Update Flow
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
