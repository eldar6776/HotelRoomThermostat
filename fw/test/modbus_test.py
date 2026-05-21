#!/usr/bin/env python3
"""
Hotel Room Thermostat - Unified Modbus RTU Test Suite
Tests all Modbus functions: Holding Registers, Input Registers, Coils, Discrete Inputs

Requirements:
    pip install pymodbus pyserial colorama

Usage:
    python modbus_test.py              # Interactive menu
    python modbus_test.py --quick       # Quick single test
    python modbus_test.py --cycles 3    # Automated N-cycle test
    python modbus_test.py --continuous  # Continuous test loop
"""

import time
import sys
import argparse
import random
from pymodbus.client import ModbusSerialClient
from pymodbus.exceptions import ModbusException
from colorama import init, Fore, Style

# Initialize colorama
init(autoreset=True)

# ── Constants ─────────────────────────────────────────────────────────────────
# Holding Registers (40001+, 0-based)
MB_REG_TARGET_TEMP = 0
MB_REG_HVAC_MODE = 1
MB_REG_FAN_SPEED = 2
MB_REG_OUTSIDE_TEMP = 3
MB_REG_UNIX_TIME_L = 4
MB_REG_UNIX_TIME_H = 5
MB_REG_TEMP_MIN = 6
MB_REG_TEMP_MAX = 7
MB_REG_HYSTERESIS = 8
MB_REG_STAGE_STEP = 9
MB_REG_SENSOR_OFFSET = 10
MB_REG_BRIGHT_HIGH = 11
MB_REG_BRIGHT_LOW = 12
MB_REG_TIMEOUT_S = 13
MB_REG_THEME_SELECT = 14
MB_REG_RELAY_MODE = 21

# Input Registers (30001+, 0-based)
MB_IREG_CURRENT_TEMP = 0
MB_IREG_TARGET_TEMP = 1
MB_IREG_RELAY_STATUS = 2
MB_IREG_UPTIME_LOW = 3
MB_IREG_UPTIME_HIGH = 4
MB_IREG_FREE_HEAP = 5
MB_IREG_WINDOW_RAW = 6
MB_IREG_SENSOR_FAULT = 7     # DODANO: Senzor fault registar

# Coils (00001+, 0-based)
MB_COIL_DND = 0
MB_COIL_MUR = 1

# Discrete Inputs (10001+, 0-based)
MB_ISTS_WINDOW_CLOSED = 0
MB_ISTS_SYSTEM_READY = 1
MB_ISTS_HVAC_ACTIVE = 2
MB_ISTS_SENSOR_FAULT = 3     # DODANO: Senzor fault alarm

# Enums
RELAY_3SPEED, RELAY_1RELAY = 0, 1
HVAC_OFF, HVAC_HEAT, HVAC_COOL = 0, 1, 2
FAN_AUTO, FAN_LOW, FAN_MID, FAN_HIGH = 0, 1, 2, 3

# ── Helper Functions sa Retry logikom ─────────────────────────────────────────
def print_header(text):
    print(f"\n{Fore.CYAN}{'=' * 70}\n{text}\n{'=' * 70}{Style.RESET_ALL}")

def print_success(text):
    print(f"{Fore.GREEN}[OK] {text}{Style.RESET_ALL}")

def print_error(text):
    print(f"{Fore.RED}[ERROR] {text}{Style.RESET_ALL}")

def print_info(text):
    print(f"{Fore.YELLOW}[INFO] {text}{Style.RESET_ALL}")

def retry_modbus(func):
    """Dekorator za retry logiku na Modbus zahtjevima"""
    def wrapper(*args, **kwargs):
        retries = 3
        for attempt in range(retries):
            try:
                result = func(*args, **kwargs)
                if result and not result.isError():
                    return result
                if attempt == retries - 1:
                    print_error(f"Failed after {retries} attempts: {result}")
            except ModbusException as e:
                if attempt == retries - 1:
                    print_error(f"Modbus exception: {e}")
            except Exception as e:
                if attempt == retries - 1:
                    print_error(f"Exception: {e}")
            time.sleep(0.1)
        return None
    return wrapper

@retry_modbus
def read_hregs(client, slave_id, address, count):
    return client.read_holding_registers(address=address, count=count, device_id=slave_id)

@retry_modbus
def read_iregs(client, slave_id, address, count):
    return client.read_input_registers(address=address, count=count, device_id=slave_id)

@retry_modbus
def read_coils(client, slave_id, address, count):
    return client.read_coils(address=address, count=count, device_id=slave_id)

@retry_modbus
def read_dists(client, slave_id, address, count):
    return client.read_discrete_inputs(address=address, count=count, device_id=slave_id)

@retry_modbus
def write_reg(client, slave_id, address, value):
    return client.write_register(address=address, value=value, device_id=slave_id)

@retry_modbus
def write_coil(client, slave_id, address, value):
    return client.write_coil(address=address, value=value, device_id=slave_id)

@retry_modbus
def write_regs(client, slave_id, address, values):
    return client.write_registers(address=address, values=values, device_id=slave_id)

# ── Test Block Functions ──────────────────────────────────────────────────────
def do_read_state(client, slave_id, verbose=True):
    """Kombinovano čitanje kompletnog stanja termostata"""
    if verbose: print_header("READING FULL THERMOSTAT STATE")
    
    hregs = read_hregs(client, slave_id, MB_REG_TARGET_TEMP, 15) # Čitamo 15 registara (0 do 14)
    rmode = read_hregs(client, slave_id, MB_REG_RELAY_MODE, 1)
    iregs = read_iregs(client, slave_id, MB_IREG_CURRENT_TEMP, 8) # Čitamo 8 registara
    coils = read_coils(client, slave_id, MB_COIL_DND, 2)
    dists = read_dists(client, slave_id, MB_ISTS_WINDOW_CLOSED, 4) # Čitamo 4 bita

    if not all([hregs, rmode, iregs, coils, dists]):
        print_error("Failed to read complete state.")
        return False

    mode_names = {0: "OFF", 1: "HEAT", 2: "COOL"}
    fan_names = {0: "AUTO", 1: "LOW", 2: "MID", 3: "HIGH"}
    relay_names = {0: "3-Speed", 1: "1-Relay"}

    print_info(f"--- Settings (Holding Regs) ---")
    print_success(f"Target Temp : {hregs.registers[0]/10.0}°C")
    print_success(f"HVAC Mode   : {mode_names.get(hregs.registers[1], 'UNK')} ({hregs.registers[1]})")
    print_success(f"Fan Speed   : {fan_names.get(hregs.registers[2], 'UNK')} ({hregs.registers[2]})")
    
    raw_out = hregs.registers[3]
    out_temp = raw_out if raw_out < 32768 else raw_out - 65536
    print_success(f"Outside Temp: {out_temp}°C")
    
    # Čitanje i dekodiranje vremena na termostatu
    low_time = hregs.registers[4]
    high_time = hregs.registers[5]
    thermo_time = (high_time << 16) | low_time
    if thermo_time > 1672531200:
        time_str = time.strftime('%Y-%m-%d %H:%M:%S', time.localtime(thermo_time))
    else:
        time_str = f"Not set / raw uptime ({thermo_time}s)"
    print_success(f"Clock Time  : {time_str}")
    
    print_success(f"Temp Min    : {hregs.registers[6]}°C")
    print_success(f"Temp Max    : {hregs.registers[7]}°C")
    print_success(f"Hysteresis  : {hregs.registers[8]/10.0}°C")
    print_success(f"Stage Step  : {hregs.registers[9]/10.0}°C")
    
    raw_offset = hregs.registers[10]
    sensor_offset = raw_offset if raw_offset < 32768 else raw_offset - 65536
    print_success(f"Sensor Offset: {sensor_offset/10.0}°C")
    
    print_success(f"Bright High : {hregs.registers[11]}")
    print_success(f"Bright Low  : {hregs.registers[12]}")
    print_success(f"Timeout S   : {hregs.registers[13]}s")
    print_success(f"Theme Select: {'LOGO' if hregs.registers[14] == 1 else 'NONE'} ({hregs.registers[14]})")
    
    print_success(f"Relay Mode  : {relay_names.get(rmode.registers[0], 'UNK')}")
    
    print_info(f"--- Sensors & System (Input Regs) ---")
    uptime = (iregs.registers[4] << 16) | iregs.registers[3]
    print_success(f"Current Temp: {iregs.registers[0]/10.0}°C")
    print_success(f"Target (Mir): {iregs.registers[1]/10.0}°C")
    print_success(f"Relay Status: 0b{iregs.registers[2]:03b}")
    print_success(f"Free Heap   : {iregs.registers[5]} KB")
    print_success(f"Uptime      : {uptime}s")
    print_success(f"Window Raw  : {'CLOSED' if iregs.registers[6] else 'OPEN'}")
    print_success(f"Sensor Fault: {'YES (FAULT)' if iregs.registers[7] else 'NO (OK)'}")

    print_info(f"--- Status Flags (Coils & Discrete) ---")
    print_success(f"DND: {'ON' if coils.bits[0] else 'OFF'} | MUR: {'ON' if coils.bits[1] else 'OFF'}")
    print_success(f"Window: {'CLOSED' if dists.bits[0] else 'OPEN'}")
    print_success(f"System Ready: {'YES' if dists.bits[1] else 'NO'}")
    print_success(f"HVAC Active: {'YES' if dists.bits[2] else 'NO'}")
    print_success(f"Fault Alarm: {'ACTIVE' if dists.bits[3] else 'CLEAR'}")

    return True

# ── Interactive Mode ──────────────────────────────────────────────────────────
def interactive_menu(client, slave_id):
    while True:
        print_header("INTERACTIVE CONTROL")
        print("1. Read Full State")
        print("2. Set Target Temperature")
        print("3. Set HVAC Mode (0=Off, 1=Heat, 2=Cool)")
        print("4. Set Fan Speed (0=Auto, 1=Low, 2=Mid, 3=High)")
        print("5. Set Relay Mode (0=3-Speed, 1=1-Relay)")
        print("6. Toggle DND (Do Not Disturb)")
        print("7. Toggle MUR (Make Up Room)")
        print("8. Set Outside Temp (e.g. -5)")
        print("9. Synchronize Time (Send PC Time)")
        print("10. Set System Configuration Settings")
        print("0. Back to Main Menu")
        
        try:
            choice = input(f"\n{Fore.CYAN}Command: {Style.RESET_ALL}").strip()
            
            if choice == '1':
                do_read_state(client, slave_id)
            elif choice == '2':
                val = float(input("Enter temp (e.g. 22.5): "))
                if write_reg(client, slave_id, MB_REG_TARGET_TEMP, int(val * 10)):
                    print_success(f"Sent {val}°C")
            elif choice == '3':
                val = int(input("Enter HVAC mode (0-2): "))
                write_reg(client, slave_id, MB_REG_HVAC_MODE, val)
            elif choice == '4':
                val = int(input("Enter Fan speed (0-3): "))
                write_reg(client, slave_id, MB_REG_FAN_SPEED, val)
            elif choice == '5':
                val = int(input("Enter Relay Mode (0-1): "))
                write_reg(client, slave_id, MB_REG_RELAY_MODE, val)
            elif choice == '6':
                val = input("Set DND (1=ON, 0=OFF): ") == '1'
                write_coil(client, slave_id, MB_COIL_DND, val)
            elif choice == '7':
                val = input("Set MUR (1=ON, 0=OFF): ") == '1'
                write_coil(client, slave_id, MB_COIL_MUR, val)
            elif choice == '8':
                val = int(input("Enter Outside Temp (-15 to 45): "))
                # Bitwise AND za dvokomplementni zapis (-5 postaje 65531 odnosno 0xFFFB)
                write_reg(client, slave_id, MB_REG_OUTSIDE_TEMP, val & 0xFFFF)
            elif choice == '9':
                curr_time = int(time.time())
                low_word = curr_time & 0xFFFF
                high_word = (curr_time >> 16) & 0xFFFF
                print_info(f"Syncing time: Sending {curr_time} ({time.strftime('%Y-%m-%d %H:%M:%S', time.localtime(curr_time))})")
                if write_regs(client, slave_id, MB_REG_UNIX_TIME_L, [low_word, high_word]):
                    print_success("Time synchronization sent successfully!")
            elif choice == '10':
                print("\nSystem Settings:")
                print("a. Set Temp Min (10 to 24 °C)")
                print("b. Set Temp Max (25 to 40 °C)")
                print("c. Set Hysteresis (0.2 to 2.0 °C)")
                print("d. Set Stage Step (0.5 to 2.5 °C)")
                print("e. Set Sensor Offset (-5.0 to 5.0 °C)")
                print("f. Set Brightness High (0 to 1023)")
                print("g. Set Brightness Low (0 to 1023)")
                print("h. Set Screensaver Timeout (30, 60, 120 s)")
                print("i. Set Theme Select (0=NONE, 1=LOGO)")
                sub_choice = input("Select setting to change (a-i): ").strip().lower()
                
                if sub_choice == 'a':
                    val = int(input("Enter Min Temp (10-24): "))
                    write_reg(client, slave_id, MB_REG_TEMP_MIN, val)
                elif sub_choice == 'b':
                    val = int(input("Enter Max Temp (25-40): "))
                    write_reg(client, slave_id, MB_REG_TEMP_MAX, val)
                elif sub_choice == 'c':
                    val = float(input("Enter Hysteresis (0.2-2.0): "))
                    write_reg(client, slave_id, MB_REG_HYSTERESIS, int(val * 10))
                elif sub_choice == 'd':
                    val = float(input("Enter Stage Step (0.5-2.5): "))
                    write_reg(client, slave_id, MB_REG_STAGE_STEP, int(val * 10))
                elif sub_choice == 'e':
                    val = float(input("Enter Sensor Offset (-5.0 to 5.0): "))
                    write_reg(client, slave_id, MB_REG_SENSOR_OFFSET, int(val * 10) & 0xFFFF)
                elif sub_choice == 'f':
                    val = int(input("Enter Brightness High (0-1023): "))
                    write_reg(client, slave_id, MB_REG_BRIGHT_HIGH, val)
                elif sub_choice == 'g':
                    val = int(input("Enter Brightness Low (0-1023): "))
                    write_reg(client, slave_id, MB_REG_BRIGHT_LOW, val)
                elif sub_choice == 'h':
                    val = int(input("Enter Screensaver Timeout (30/60/120): "))
                    write_reg(client, slave_id, MB_REG_TIMEOUT_S, val)
                elif sub_choice == 'i':
                    val = int(input("Enter Theme Select (0=NONE, 1=LOGO): "))
                    write_reg(client, slave_id, MB_REG_THEME_SELECT, val)
            elif choice == '0':
                break
        except ValueError:
            print_error("Invalid input")
        
        time.sleep(0.5)

# ── Auto Test Mode ────────────────────────────────────────────────────────────
def run_automated_test(client, slave_id, cycles):
    # Automatska sinhronizacija vremena na početku testa
    curr_time = int(time.time())
    write_regs(client, slave_id, MB_REG_UNIX_TIME_L, [curr_time & 0xFFFF, (curr_time >> 16) & 0xFFFF])
    print_success("Auto-synced time at test start.")

    for cycle in range(1, cycles + 1):
        print_header(f"CYCLE {cycle}/{cycles}")
        
        # Generiši slučajne validne parametre za nove NVS registre
        t_min = random.randint(10, 24)
        t_max = random.randint(25, 40)
        hyst = random.randint(2, 20)      # Hysteresis x10 (0.2 do 2.0)
        stage = random.randint(5, 25)     # Stage step x10 (0.5 do 2.5)
        offset = random.randint(-50, 50)  # Offset x10 (-5.0 do 5.0)
        b_high = random.randint(500, 1023)
        b_low = random.randint(0, 499)
        tout = random.choice([30, 60, 120])
        theme = random.choice([0, 1])

        print_info("Writing random new settings parameters via Modbus...")
        write_reg(client, slave_id, MB_REG_TEMP_MIN, t_min)
        write_reg(client, slave_id, MB_REG_TEMP_MAX, t_max)
        write_reg(client, slave_id, MB_REG_HYSTERESIS, hyst)
        write_reg(client, slave_id, MB_REG_STAGE_STEP, stage)
        write_reg(client, slave_id, MB_REG_SENSOR_OFFSET, offset & 0xFFFF)
        write_reg(client, slave_id, MB_REG_BRIGHT_HIGH, b_high)
        write_reg(client, slave_id, MB_REG_BRIGHT_LOW, b_low)
        write_reg(client, slave_id, MB_REG_TIMEOUT_S, tout)
        write_reg(client, slave_id, MB_REG_THEME_SELECT, theme)
        
        temps = [20.0, 22.5, 24.0]
        modes = [HVAC_OFF, HVAC_HEAT, HVAC_COOL]
        fans = [FAN_AUTO, FAN_LOW, FAN_MID, FAN_HIGH]
        rmodes = [RELAY_3SPEED, RELAY_1RELAY]
        
        out_temp = random.randint(-15, 45)
        
        # Write sequence
        write_reg(client, slave_id, MB_REG_TARGET_TEMP, int(temps[cycle % 3] * 10))
        write_reg(client, slave_id, MB_REG_HVAC_MODE, modes[cycle % 3])
        write_reg(client, slave_id, MB_REG_FAN_SPEED, fans[cycle % 4])
        write_reg(client, slave_id, MB_REG_OUTSIDE_TEMP, out_temp & 0xFFFF)
        write_reg(client, slave_id, MB_REG_RELAY_MODE, rmodes[cycle % 2])
        write_coil(client, slave_id, MB_COIL_DND, cycle % 2 == 0)
        
        time.sleep(1.0)

        # Čitanje i verifikacija
        print_info("Verifying written parameters...")
        hregs = read_hregs(client, slave_id, MB_REG_TARGET_TEMP, 15)
        if not hregs:
            print_error("Failed to read back holding registers for verification.")
            continue
            
        # Pomoćna funkcija za poređenje
        def verify_param(name, got, expected):
            if got == expected:
                print_success(f"{name} successfully verified: {got}")
            else:
                print_error(f"{name} verification FAILED! Got {got}, expected {expected}")

        verify_param("Temp Min", hregs.registers[MB_REG_TEMP_MIN], t_min)
        verify_param("Temp Max", hregs.registers[MB_REG_TEMP_MAX], t_max)
        verify_param("Hysteresis", hregs.registers[MB_REG_HYSTERESIS], hyst)
        verify_param("Stage Step", hregs.registers[MB_REG_STAGE_STEP], stage)
        
        raw_offset_read = hregs.registers[MB_REG_SENSOR_OFFSET]
        offset_read = raw_offset_read if raw_offset_read < 32768 else raw_offset_read - 65536
        verify_param("Sensor Offset", offset_read, offset)
        
        verify_param("Bright High", hregs.registers[MB_REG_BRIGHT_HIGH], b_high)
        verify_param("Bright Low", hregs.registers[MB_REG_BRIGHT_LOW], b_low)
        verify_param("Timeout S", hregs.registers[MB_REG_TIMEOUT_S], tout)
        verify_param("Theme Select", hregs.registers[MB_REG_THEME_SELECT], theme)

        do_read_state(client, slave_id, verbose=False)
        
# ── Main ──────────────────────────────────────────────────────────────────────
def main():
    parser = argparse.ArgumentParser(description='Thermostat Modbus Robust Test')
    parser.add_argument('--port', default='COM6')
    parser.add_argument('--slave', type=int, default=1)
    parser.add_argument('--baudrate', type=int, default=115200)
    parser.add_argument('--quick', action='store_true')
    parser.add_argument('--cycles', type=int)
    parser.add_argument('--continuous', action='store_true')
    args = parser.parse_args()

    client = ModbusSerialClient(
        port=args.port, baudrate=args.baudrate,
        bytesize=8, parity='N', stopbits=1, timeout=1.0
    )
    
    if not client.connect():
        print_error(f"Failed to connect to {args.port}")
        return 1

    try:
        if args.quick:
            run_automated_test(client, args.slave, 1)
        elif args.cycles:
            run_automated_test(client, args.slave, args.cycles)
        elif args.continuous:
            c = 1
            while True:
                run_automated_test(client, args.slave, c)
                c += 1
        else:
            while True:
                print_header("MAIN MENU")
                print("1. Interactive Control (Manual commands)")
                print("2. Run Quick Automated Test (1 cycle)")
                print("3. Run Continuous Stress Test")
                print("0. Exit")
                
                c = input(f"\n{Fore.CYAN}Choice: {Style.RESET_ALL}").strip()
                if c == '1':
                    interactive_menu(client, args.slave)
                elif c == '2':
                    run_automated_test(client, args.slave, 1)
                elif c == '3':
                    try:
                        cycles = int(input("How many cycles? "))
                        run_automated_test(client, args.slave, cycles)
                    except ValueError:
                        print_error("Invalid number")
                elif c == '0':
                    break
    except KeyboardInterrupt:
        print("\nTest interrupted.")
    finally:
        client.close()

if __name__ == "__main__":
    sys.exit(main())
