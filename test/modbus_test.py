#!/usr/bin/env python3
"""
Modbus RTU Test Script for Hotel Room Thermostat
Tests all Modbus functions: Holding Registers, Input Registers, Coils, Discrete Inputs

Requirements:
    pip install pymodbus pyserial colorama

Usage:
    python modbus_test.py
"""

import time
import sys
from pymodbus.client import ModbusSerialClient
from pymodbus.exceptions import ModbusException
from colorama import init, Fore, Style

# Initialize colorama for colored console output
init(autoreset=True)

# ── Constants ─────────────────────────────────────────────────────────────────
# Holding Registers (40001+, 0-based addressing)
MB_REG_TARGET_TEMP = 0      # 40001
MB_REG_HVAC_MODE = 1        # 40002
MB_REG_FAN_SPEED = 2        # 40003
MB_REG_RELAY_MODE = 21      # 40022
MB_REG_WX_WATCHDOG = 29     # 40030

# Input Registers (30001+, 0-based addressing)
MB_IREG_CURRENT_TEMP = 0    # 30001
MB_IREG_TARGET_TEMP = 1     # 30002
MB_IREG_RELAY_STATUS = 2    # 30003
MB_IREG_UPTIME_LOW = 3      # 30004
MB_IREG_UPTIME_HIGH = 4     # 30005
MB_IREG_FREE_HEAP = 5       # 30006
MB_IREG_WINDOW_RAW = 6      # 30007

# Coils (00001+, 0-based addressing)
MB_COIL_DND = 0             # 00001
MB_COIL_MUR = 1             # 00002

# Discrete Inputs (10001+, 0-based addressing)
MB_ISTS_WINDOW_CLOSED = 0   # 10001
MB_ISTS_SYSTEM_READY = 1    # 10002
MB_ISTS_HVAC_ACTIVE = 2     # 10003
MB_ISTS_WEATHER_VALID = 3   # 10004

# HVAC Modes
HVAC_OFF = 0
HVAC_HEAT = 1
HVAC_COOL = 2

# Fan Speeds
FAN_AUTO = 0
FAN_LOW = 1
FAN_MID = 2
FAN_HIGH = 3

# ── Helper Functions ──────────────────────────────────────────────────────────
def print_header(text):
    """Print colored section header"""
    print(f"\n{Fore.CYAN}{'=' * 70}")
    print(f"{Fore.CYAN}{text}")
    print(f"{Fore.CYAN}{'=' * 70}{Style.RESET_ALL}")

def print_success(text):
    """Print success message"""
    print(f"{Fore.GREEN}[OK] {text}{Style.RESET_ALL}")

def print_error(text):
    """Print error message"""
    print(f"{Fore.RED}[ERROR] {text}{Style.RESET_ALL}")

def print_info(text):
    """Print info message"""
    print(f"{Fore.YELLOW}[INFO] {text}{Style.RESET_ALL}")

def safe_read_holding_registers(client, slave_id, address, count):
    """Safely read holding registers with error handling"""
    try:
        result = client.read_holding_registers(address=address, count=count, device_id=slave_id)
        if result.isError():
            print_error(f"Failed to read holding registers at {address}")
            return None
        return result.registers
    except ModbusException as e:
        print_error(f"Modbus exception: {e}")
        return None
    except Exception as e:
        print_error(f"Exception: {e}")
        return None

def safe_read_input_registers(client, slave_id, address, count):
    """Safely read input registers with error handling"""
    try:
        result = client.read_input_registers(address=address, count=count, device_id=slave_id)
        if result.isError():
            print_error(f"Failed to read input registers at {address}")
            return None
        return result.registers
    except ModbusException as e:
        print_error(f"Modbus exception: {e}")
        return None
    except Exception as e:
        print_error(f"Exception: {e}")
        return None

def safe_write_register(client, slave_id, address, value):
    """Safely write single holding register with error handling"""
    try:
        result = client.write_register(address=address, value=value, device_id=slave_id)
        if result.isError():
            print_error(f"Failed to write register {address} = {value}")
            return False
        return True
    except ModbusException as e:
        print_error(f"Modbus exception: {e}")
        return False
    except Exception as e:
        print_error(f"Exception: {e}")
        return False

def safe_read_coils(client, slave_id, address, count):
    """Safely read coils with error handling"""
    try:
        result = client.read_coils(address=address, count=count, device_id=slave_id)
        if result.isError():
            print_error(f"Failed to read coils at {address}")
            return None
        return result.bits[:count]
    except ModbusException as e:
        print_error(f"Modbus exception: {e}")
        return None
    except Exception as e:
        print_error(f"Exception: {e}")
        return None

def safe_write_coil(client, slave_id, address, value):
    """Safely write single coil with error handling"""
    try:
        result = client.write_coil(address=address, value=value, device_id=slave_id)
        if result.isError():
            print_error(f"Failed to write coil {address} = {value}")
            return False
        return True
    except ModbusException as e:
        print_error(f"Modbus exception: {e}")
        return False
    except Exception as e:
        print_error(f"Exception: {e}")
        return False

def safe_read_discrete_inputs(client, slave_id, address, count):
    """Safely read discrete inputs with error handling"""
    try:
        result = client.read_discrete_inputs(address=address, count=count, device_id=slave_id)
        if result.isError():
            print_error(f"Failed to read discrete inputs at {address}")
            return None
        return result.bits[:count]
    except ModbusException as e:
        print_error(f"Modbus exception: {e}")
        return None
    except Exception as e:
        print_error(f"Exception: {e}")
        return None

# ── Test Functions ────────────────────────────────────────────────────────────
def test_read_holding_registers(client, slave_id):
    """Test reading holding registers"""
    print_header("TEST 1: Read Holding Registers (40001-40003)")
    
    regs = safe_read_holding_registers(client, slave_id, MB_REG_TARGET_TEMP, 3)
    if regs:
        target_temp = regs[0] / 10.0
        hvac_mode = regs[1]
        fan_speed = regs[2]
        
        mode_names = {0: "OFF", 1: "HEAT", 2: "COOL"}
        fan_names = {0: "AUTO", 1: "LOW", 2: "MID", 3: "HIGH"}
        
        print_success(f"Target Temperature: {target_temp:.1f} °C (raw: {regs[0]})")
        print_success(f"HVAC Mode: {mode_names.get(hvac_mode, 'UNKNOWN')} ({hvac_mode})")
        print_success(f"Fan Speed: {fan_names.get(fan_speed, 'UNKNOWN')} ({fan_speed})")
        return True
    return False

def test_read_input_registers(client, slave_id):
    """Test reading input registers"""
    print_header("TEST 2: Read Input Registers (30001-30007)")
    
    regs = safe_read_input_registers(client, slave_id, MB_IREG_CURRENT_TEMP, 7)
    if regs:
        current_temp = regs[0] / 10.0
        target_temp = regs[1] / 10.0
        relay_status = regs[2]
        uptime = (regs[4] << 16) | regs[3]
        free_heap = regs[5]
        window_raw = regs[6]
        
        print_success(f"Current Temperature: {current_temp:.1f} °C (raw: {regs[0]})")
        print_success(f"Target Temperature (mirror): {target_temp:.1f} °C")
        print_success(f"Relay Status: 0b{relay_status:03b} (R1={bool(relay_status&1)}, R2={bool(relay_status&2)}, R3={bool(relay_status&4)})")
        print_success(f"Uptime: {uptime} seconds ({uptime/3600:.2f} hours)")
        print_success(f"Free Heap: {free_heap} KB")
        print_success(f"Window Sensor: {'CLOSED' if window_raw else 'OPEN'}")
        return True
    return False

def test_read_coils(client, slave_id):
    """Test reading coils"""
    print_header("TEST 3: Read Coils (00001-00002)")
    
    coils = safe_read_coils(client, slave_id, MB_COIL_DND, 2)
    if coils:
        print_success(f"DND (Do Not Disturb): {'ON' if coils[0] else 'OFF'}")
        print_success(f"MUR (Make Up Room): {'ON' if coils[1] else 'OFF'}")
        return True
    return False

def test_read_discrete_inputs(client, slave_id):
    """Test reading discrete inputs"""
    print_header("TEST 4: Read Discrete Inputs (10001-10004)")
    
    inputs = safe_read_discrete_inputs(client, slave_id, MB_ISTS_WINDOW_CLOSED, 4)
    if inputs:
        print_success(f"Window Sensor: {'CLOSED' if inputs[0] else 'OPEN'}")
        print_success(f"System Ready: {'YES' if inputs[1] else 'NO'}")
        print_success(f"HVAC Active: {'YES' if inputs[2] else 'NO'}")
        print_success(f"Weather Valid: {'YES' if inputs[3] else 'NO'}")
        return True
    return False

def test_write_target_temp(client, slave_id, temp_c):
    """Test writing target temperature"""
    print_header(f"TEST 5: Write Target Temperature ({temp_c}°C)")
    
    temp_x10 = int(temp_c * 10)
    if safe_write_register(client, slave_id, MB_REG_TARGET_TEMP, temp_x10):
        time.sleep(0.2)  # Wait for processing
        regs = safe_read_holding_registers(client, slave_id, MB_REG_TARGET_TEMP, 1)
        if regs and regs[0] == temp_x10:
            print_success(f"Target temperature set to {temp_c}°C (verified)")
            return True
        else:
            print_error(f"Verification failed: expected {temp_x10}, got {regs[0] if regs else 'N/A'}")
    return False

def test_write_hvac_mode(client, slave_id, mode):
    """Test writing HVAC mode"""
    mode_names = {HVAC_OFF: "OFF", HVAC_HEAT: "HEAT", HVAC_COOL: "COOL"}
    print_header(f"TEST 6: Write HVAC Mode ({mode_names.get(mode, 'UNKNOWN')})")
    
    if safe_write_register(client, slave_id, MB_REG_HVAC_MODE, mode):
        time.sleep(0.2)
        regs = safe_read_holding_registers(client, slave_id, MB_REG_HVAC_MODE, 1)
        if regs and regs[0] == mode:
            print_success(f"HVAC mode set to {mode_names.get(mode, 'UNKNOWN')} (verified)")
            return True
        else:
            print_error(f"Verification failed: expected {mode}, got {regs[0] if regs else 'N/A'}")
    return False

def test_write_fan_speed(client, slave_id, speed):
    """Test writing fan speed"""
    fan_names = {FAN_AUTO: "AUTO", FAN_LOW: "LOW", FAN_MID: "MID", FAN_HIGH: "HIGH"}
    print_header(f"TEST 7: Write Fan Speed ({fan_names.get(speed, 'UNKNOWN')})")
    
    if safe_write_register(client, slave_id, MB_REG_FAN_SPEED, speed):
        time.sleep(0.2)
        regs = safe_read_holding_registers(client, slave_id, MB_REG_FAN_SPEED, 1)
        if regs and regs[0] == speed:
            print_success(f"Fan speed set to {fan_names.get(speed, 'UNKNOWN')} (verified)")
            return True
        else:
            print_error(f"Verification failed: expected {speed}, got {regs[0] if regs else 'N/A'}")
    return False

def test_write_coils(client, slave_id):
    """Test writing coils (DND/MUR)"""
    print_header("TEST 8: Write Coils (DND & MUR)")
    
    # Test DND ON
    if safe_write_coil(client, slave_id, MB_COIL_DND, True):
        time.sleep(0.2)
        coils = safe_read_coils(client, slave_id, MB_COIL_DND, 1)
        if coils and coils[0]:
            print_success("DND set to ON (verified)")
        else:
            print_error("DND verification failed")
            return False
    
    # Test MUR ON
    if safe_write_coil(client, slave_id, MB_COIL_MUR, True):
        time.sleep(0.2)
        coils = safe_read_coils(client, slave_id, MB_COIL_MUR, 1)
        if coils and coils[0]:
            print_success("MUR set to ON (verified)")
        else:
            print_error("MUR verification failed")
            return False
    
    # Test DND OFF
    if safe_write_coil(client, slave_id, MB_COIL_DND, False):
        time.sleep(0.2)
        coils = safe_read_coils(client, slave_id, MB_COIL_DND, 1)
        if coils and not coils[0]:
            print_success("DND set to OFF (verified)")
        else:
            print_error("DND OFF verification failed")
            return False
    
    return True

def test_weather_data_write(client, slave_id):
    """Test writing weather forecast data"""
    print_header("TEST 9: Write Weather Forecast Data")
    
    # Write weather watchdog trigger
    if not safe_write_register(client, slave_id, MB_REG_WX_WATCHDOG, 1):
        return False
    print_info("Weather watchdog triggered")
    
    # Write 5 days of weather data (3 registers per day)
    # Format: reg+0 = day_id | (icon_id << 8), reg+1 = temp_high_x10, reg+2 = temp_low_x10
    weather_data = [
        # Day 0: Monday, Sunny (icon 0), High 25°C, Low 18°C
        (1 | (0 << 8), 250, 180),
        # Day 1: Tuesday, Heating (icon 1), High 22°C, Low 15°C
        (2 | (1 << 8), 220, 150),
        # Day 2: Wednesday, Cooling (icon 2), High 28°C, Low 20°C
        (3 | (2 << 8), 280, 200),
        # Day 3: Thursday, Sunny, High 26°C, Low 19°C
        (4 | (0 << 8), 260, 190),
        # Day 4: Friday, Sunny, High 24°C, Low 17°C
        (5 | (0 << 8), 240, 170),
    ]
    
    base_addr = 30  # MB_REG_WX_BASE (0-based)
    for day_idx, (packed, temp_high, temp_low) in enumerate(weather_data):
        addr = base_addr + day_idx * 3
        if not safe_write_register(client, slave_id, addr, packed):
            return False
        if not safe_write_register(client, slave_id, addr + 1, temp_high):
            return False
        if not safe_write_register(client, slave_id, addr + 2, temp_low):
            return False
        
        day_id = packed & 0xFF
        icon_id = (packed >> 8) & 0xFF
        day_names = ["Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"]
        print_success(f"Day {day_idx}: {day_names[day_id]}, Icon {icon_id}, High {temp_high/10:.1f}°C, Low {temp_low/10:.1f}°C")
    
    return True

def run_continuous_test(client, slave_id, interval=2.0):
    """Run all tests continuously with specified interval"""
    print_info(f"Starting continuous test loop (press Ctrl+C to stop)")
    print_info(f"Test interval: {interval} seconds\n")
    
    test_count = 0
    try:
        while True:
            test_count += 1
            print(f"\n{Fore.MAGENTA}{'=' * 70}")
            print(f"{Fore.MAGENTA}Test Cycle #{test_count}")
            print(f"{Fore.MAGENTA}{'=' * 70}{Style.RESET_ALL}")
            
            # Read tests
            test_read_holding_registers(client, slave_id)
            time.sleep(0.5)
            
            test_read_input_registers(client, slave_id)
            time.sleep(0.5)
            
            test_read_coils(client, slave_id)
            time.sleep(0.5)
            
            test_read_discrete_inputs(client, slave_id)
            time.sleep(0.5)
            
            # Write tests (cycle through values)
            temps = [20.0, 22.0, 24.0, 21.5]
            test_write_target_temp(client, slave_id, temps[test_count % len(temps)])
            time.sleep(0.5)
            
            modes = [HVAC_OFF, HVAC_HEAT, HVAC_COOL]
            test_write_hvac_mode(client, slave_id, modes[test_count % len(modes)])
            time.sleep(0.5)
            
            speeds = [FAN_AUTO, FAN_LOW, FAN_MID, FAN_HIGH]
            test_write_fan_speed(client, slave_id, speeds[test_count % len(speeds)])
            time.sleep(0.5)
            
            test_write_coils(client, slave_id)
            time.sleep(0.5)
            
            # Weather test every 5 cycles
            if test_count % 5 == 0:
                test_weather_data_write(client, slave_id)
                time.sleep(0.5)
            
            print_info(f"Waiting {interval} seconds before next cycle...")
            time.sleep(interval)
            
    except KeyboardInterrupt:
        print(f"\n{Fore.YELLOW}Test stopped by user.{Style.RESET_ALL}")

# ── Main ──────────────────────────────────────────────────────────────────────
def main():
    """Main entry point"""
    print_header("Hotel Room Thermostat - Modbus RTU Test Script")
    
    # Get user input
    try:
        port = input(f"{Fore.CYAN}Enter COM port (e.g., COM6): {Style.RESET_ALL}").strip()
        if not port:
            print_error("COM port cannot be empty")
            sys.exit(1)
        
        slave_id = int(input(f"{Fore.CYAN}Enter Modbus slave address (1-247): {Style.RESET_ALL}"))
        if not (1 <= slave_id <= 247):
            print_error("Slave address must be between 1 and 247")
            sys.exit(1)
            
        baudrate = 115200  # Fixed to match firmware
        interval = float(input(f"{Fore.CYAN}Enter test interval in seconds (default 2.0): {Style.RESET_ALL}") or "2.0")
        
    except ValueError as e:
        print_error(f"Invalid input: {e}")
        sys.exit(1)
    except KeyboardInterrupt:
        print(f"\n{Fore.YELLOW}Cancelled by user.{Style.RESET_ALL}")
        sys.exit(0)
    
    # Connect to Modbus device
    print_info(f"Connecting to {port} at {baudrate} baud, slave ID {slave_id}...")
    
    client = ModbusSerialClient(
        port=port,
        baudrate=baudrate,
        bytesize=8,
        parity='N',
        stopbits=1,
        timeout=1.0
    )
    
    if not client.connect():
        print_error(f"Failed to connect to {port}")
        sys.exit(1)
    
    print_success(f"Connected to {port}")
    
    try:
        # Run continuous test loop
        run_continuous_test(client, slave_id, interval)
    finally:
        client.close()
        print_info("Connection closed")

if __name__ == "__main__":
    main()
