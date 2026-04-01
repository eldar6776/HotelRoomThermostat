#!/usr/bin/env python3
"""
Automated Modbus test - runs without user input
"""
import sys
import os

# Add parent directory to path
sys.path.insert(0, os.path.dirname(__file__))

# Import test functions directly
import time
from pymodbus.client import ModbusSerialClient
from pymodbus.exceptions import ModbusException
from colorama import init, Fore, Style

init(autoreset=True)

# Test parameters
COM_PORT = "COM6"
SLAVE_ID = 1
BAUDRATE = 115200
TEST_CYCLES = 3

# Import constants from modbus_test
MB_REG_TARGET_TEMP = 0
MB_REG_HVAC_MODE = 1
MB_REG_FAN_SPEED = 2
MB_IREG_CURRENT_TEMP = 0
MB_IREG_RELAY_STATUS = 2
MB_COIL_DND = 0
MB_COIL_MUR = 1
MB_ISTS_WINDOW_CLOSED = 0

def main():
    print(f"{Fore.CYAN}{'=' * 70}")
    print(f"{Fore.CYAN}Automated Modbus Test - {COM_PORT} @ {BAUDRATE} baud, Slave {SLAVE_ID}")
    print(f"{Fore.CYAN}{'=' * 70}{Style.RESET_ALL}\\n")
    
    # Connect
    client = ModbusSerialClient(
        port=COM_PORT,
        baudrate=BAUDRATE,
        bytesize=8,
        parity='N',
        stopbits=1,
        timeout=1.0
    )
    
    if not client.connect():
        print(f"{Fore.RED}[ERROR] Failed to connect to {COM_PORT}{Style.RESET_ALL}")
        return 1
    
    print(f"{Fore.GREEN}[OK] Connected to {COM_PORT}{Style.RESET_ALL}\\n")
    
    try:
        for cycle in range(1, TEST_CYCLES + 1):
            print(f"{Fore.MAGENTA}{'=' * 70}")
            print(f"{Fore.MAGENTA}Test Cycle #{cycle}/{TEST_CYCLES}")
            print(f"{Fore.MAGENTA}{'=' * 70}{Style.RESET_ALL}")
            
            # Test 1: Read holding registers
            print(f"\\n{Fore.CYAN}TEST: Read Holding Registers{Style.RESET_ALL}")
            try:
                result = client.read_holding_registers(0, count=3, device_id=SLAVE_ID)
                if not result.isError():
                    temp = result.registers[0] / 10.0
                    mode = result.registers[1]
                    fan = result.registers[2]
                    print(f"{Fore.GREEN}[OK] Target Temp: {temp:.1f}°C, Mode: {mode}, Fan: {fan}{Style.RESET_ALL}")
                else:
                    print(f"{Fore.RED}[ERROR] Read failed{Style.RESET_ALL}")
            except Exception as e:
                print(f"{Fore.RED}[ERROR] {e}{Style.RESET_ALL}")
            
            time.sleep(0.5)
            
            # Test 2: Read input registers
            print(f"\\n{Fore.CYAN}TEST: Read Input Registers{Style.RESET_ALL}")
            try:
                result = client.read_input_registers(0, count=7, device_id=SLAVE_ID)
                if not result.isError():
                    current_temp = result.registers[0] / 10.0
                    relay_status = result.registers[2]
                    uptime = (result.registers[4] << 16) | result.registers[3]
                    print(f"{Fore.GREEN}[OK] Current Temp: {current_temp:.1f}°C, Relays: 0b{relay_status:03b}, Uptime: {uptime}s{Style.RESET_ALL}")
                else:
                    print(f"{Fore.RED}[ERROR] Read failed{Style.RESET_ALL}")
            except Exception as e:
                print(f"{Fore.RED}[ERROR] {e}{Style.RESET_ALL}")
            
            time.sleep(0.5)
            
            # Test 3: Write holding register (temp setpoint)
            new_temp = 200 + cycle * 10  # 20.0, 21.0, 22.0...
            print(f"\\n{Fore.CYAN}TEST: Write Target Temp = {new_temp/10:.1f}°C{Style.RESET_ALL}")
            try:
                result = client.write_register(0, new_temp, device_id=SLAVE_ID)
                if not result.isError():
                    # Verify
                    verify = client.read_holding_registers(0, count=1, device_id=SLAVE_ID)
                    if not verify.isError() and verify.registers[0] == new_temp:
                        print(f"{Fore.GREEN}[OK] Write verified{Style.RESET_ALL}")
                    else:
                        print(f"{Fore.YELLOW}[WARN] Write succeeded but verify failed{Style.RESET_ALL}")
                else:
                    print(f"{Fore.RED}[ERROR] Write failed{Style.RESET_ALL}")
            except Exception as e:
                print(f"{Fore.RED}[ERROR] {e}{Style.RESET_ALL}")
            
            time.sleep(0.5)
            
            # Test 4: Read coils
            print(f"\\n{Fore.CYAN}TEST: Read Coils{Style.RESET_ALL}")
            try:
                result = client.read_coils(0, count=2, device_id=SLAVE_ID)
                if not result.isError():
                    dnd = result.bits[0]
                    mur = result.bits[1]
                    print(f"{Fore.GREEN}[OK] DND: {'ON' if dnd else 'OFF'}, MUR: {'ON' if mur else 'OFF'}{Style.RESET_ALL}")
                else:
                    print(f"{Fore.RED}[ERROR] Read failed{Style.RESET_ALL}")
            except Exception as e:
                print(f"{Fore.RED}[ERROR] {e}{Style.RESET_ALL}")
            
            time.sleep(1.5)
            print()
        
        print(f"\\n{Fore.GREEN}{'=' * 70}")
        print(f"{Fore.GREEN}All {TEST_CYCLES} test cycles completed!")
        print(f"{Fore.GREEN}{'=' * 70}{Style.RESET_ALL}")
        
    finally:
        client.close()
        print(f"\\n{Fore.YELLOW}[INFO] Connection closed{Style.RESET_ALL}")
    
    return 0

if __name__ == "__main__":
    sys.exit(main())
