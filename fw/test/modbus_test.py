#!/usr/bin/env python3
"""
Hotel Room Thermostat - Unified Modbus RTU Test Suite
Tests all Modbus functions: Holding Registers, Input Registers, Coils, Discrete Inputs

Requirements:
    pip install pymodbus pyserial colorama

Usage:
    python modbus_test.py              # Interactive menu
    python modbus_test.py --quick       # Quick single test (auto-detect COM6, slave 1)
    python modbus_test.py --cycles 3    # Automated N-cycle test
    python modbus_test.py --continuous  # Continuous test loop
    python modbus_test.py --port COM6 --slave 1 --quick
"""

import time
import sys
import argparse
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
MB_REG_RELAY_MODE = 21      # 40022 – 0=3-speed, 1=1-relay
MB_HREG_COUNT = 22          # total holding registers (0-21)

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

# Relay Mode values (MB_REG_RELAY_MODE)
RELAY_3SPEED = 0   # 3-speed fan relay control
RELAY_1RELAY = 1   # single on/off relay control

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
def test_read_holding_registers(client, slave_id, verbose=True):
    """Test reading holding registers"""
    if verbose:
        print_header("TEST: Read Holding Registers (40001-40003, 40022)")
    
    regs = safe_read_holding_registers(client, slave_id, MB_REG_TARGET_TEMP, 3)
    if regs is None:
        return False

    target_temp = regs[0] / 10.0
    hvac_mode = regs[1]
    fan_speed = regs[2]

    mode_names = {0: "OFF", 1: "HEAT", 2: "COOL"}
    fan_names = {0: "AUTO", 1: "LOW", 2: "MID", 3: "HIGH"}
    relay_mode_names = {RELAY_3SPEED: "3-speed", RELAY_1RELAY: "1-relay"}

    # Read relay mode separately (register 21 – gap after reg 2)
    relay_regs = safe_read_holding_registers(client, slave_id, MB_REG_RELAY_MODE, 1)
    relay_mode = relay_regs[0] if relay_regs is not None else None

    if verbose:
        print_success(f"Target Temperature: {target_temp:.1f} °C (raw: {regs[0]})")
        print_success(f"HVAC Mode: {mode_names.get(hvac_mode, 'UNKNOWN')} ({hvac_mode})")
        print_success(f"Fan Speed: {fan_names.get(fan_speed, 'UNKNOWN')} ({fan_speed})")
        if relay_mode is not None:
            print_success(f"Relay Mode: {relay_mode_names.get(relay_mode, 'UNKNOWN')} ({relay_mode})")
    else:
        relay_str = relay_mode_names.get(relay_mode, '?') if relay_mode is not None else '?'
        print_success(f"Target Temp: {target_temp:.1f}°C, Mode: {hvac_mode}, Fan: {fan_speed}, Relay: {relay_str}")
    return True

def test_read_input_registers(client, slave_id, verbose=True):
    """Test reading input registers"""
    if verbose:
        print_header("TEST: Read Input Registers (30001-30007)")
    
    regs = safe_read_input_registers(client, slave_id, MB_IREG_CURRENT_TEMP, 7)
    if regs:
        current_temp = regs[0] / 10.0
        target_temp = regs[1] / 10.0
        relay_status = regs[2]
        uptime = (regs[4] << 16) | regs[3]
        free_heap = regs[5]
        window_raw = regs[6]
        
        if verbose:
            print_success(f"Current Temperature: {current_temp:.1f} °C (raw: {regs[0]})")
            print_success(f"Target Temperature (mirror): {target_temp:.1f} °C")
            print_success(f"Relay Status: 0b{relay_status:03b} (R1={bool(relay_status&1)}, R2={bool(relay_status&2)}, R3={bool(relay_status&4)})")
            print_success(f"Uptime: {uptime} seconds ({uptime/3600:.2f} hours)")
            print_success(f"Free Heap: {free_heap} KB")
            print_success(f"Window Sensor: {'CLOSED' if window_raw else 'OPEN'}")
        else:
            print_success(f"Current Temp: {current_temp:.1f}°C, Relays: 0b{relay_status:03b}, Uptime: {uptime}s")
        return True
    return False

def test_read_coils(client, slave_id, verbose=True):
    """Test reading coils"""
    if verbose:
        print_header("TEST: Read Coils (00001-00002)")
    
    coils = safe_read_coils(client, slave_id, MB_COIL_DND, 2)
    if coils:
        if verbose:
            print_success(f"DND (Do Not Disturb): {'ON' if coils[0] else 'OFF'}")
            print_success(f"MUR (Make Up Room): {'ON' if coils[1] else 'OFF'}")
        else:
            print_success(f"DND: {'ON' if coils[0] else 'OFF'}, MUR: {'ON' if coils[1] else 'OFF'}")
        return True
    return False

def test_read_discrete_inputs(client, slave_id, verbose=True):
    """Test reading discrete inputs"""
    if verbose:
        print_header("TEST: Read Discrete Inputs (10001-10003)")

    inputs = safe_read_discrete_inputs(client, slave_id, MB_ISTS_WINDOW_CLOSED, 3)
    if inputs:
        print_success(f"Window Sensor: {'CLOSED' if inputs[0] else 'OPEN'}")
        print_success(f"System Ready: {'YES' if inputs[1] else 'NO'}")
        print_success(f"HVAC Active: {'YES' if inputs[2] else 'NO'}")
        return True
    return False

def test_write_target_temp(client, slave_id, temp_c, verbose=True):
    """Test writing target temperature"""
    if verbose:
        print_header(f"TEST: Write Target Temperature ({temp_c}°C)")
    else:
        print(f"\n{Fore.CYAN}TEST: Write Target Temp = {temp_c}°C{Style.RESET_ALL}")
    
    temp_x10 = int(temp_c * 10)
    if safe_write_register(client, slave_id, MB_REG_TARGET_TEMP, temp_x10):
        time.sleep(0.2)  # Wait for processing
        regs = safe_read_holding_registers(client, slave_id, MB_REG_TARGET_TEMP, 1)
        if regs and regs[0] == temp_x10:
            print_success(f"Write verified" if not verbose else f"Target temperature set to {temp_c}°C (verified)")
            return True
        else:
            print_error(f"Verification failed: expected {temp_x10}, got {regs[0] if regs else 'N/A'}")
    return False

def test_write_hvac_mode(client, slave_id, mode, verbose=True):
    """Test writing HVAC mode"""
    mode_names = {HVAC_OFF: "OFF", HVAC_HEAT: "HEAT", HVAC_COOL: "COOL"}
    if verbose:
        print_header(f"TEST: Write HVAC Mode ({mode_names.get(mode, 'UNKNOWN')})")
    
    if safe_write_register(client, slave_id, MB_REG_HVAC_MODE, mode):
        time.sleep(0.2)
        regs = safe_read_holding_registers(client, slave_id, MB_REG_HVAC_MODE, 1)
        if regs and regs[0] == mode:
            print_success(f"HVAC mode set to {mode_names.get(mode, 'UNKNOWN')} (verified)")
            return True
        else:
            print_error(f"Verification failed: expected {mode}, got {regs[0] if regs else 'N/A'}")
    return False

def test_write_relay_mode(client, slave_id, mode, verbose=True):
    """Test writing relay control mode (MB_REG_RELAY_MODE = 40022)"""
    relay_mode_names = {RELAY_3SPEED: "3-speed", RELAY_1RELAY: "1-relay"}
    if verbose:
        print_header(f"TEST: Write Relay Mode ({relay_mode_names.get(mode, 'UNKNOWN')})")

    if safe_write_register(client, slave_id, MB_REG_RELAY_MODE, mode):
        time.sleep(0.2)
        regs = safe_read_holding_registers(client, slave_id, MB_REG_RELAY_MODE, 1)
        if regs and regs[0] == mode:
            print_success(f"Relay mode set to {relay_mode_names.get(mode, 'UNKNOWN')} (verified)")
            return True
        else:
            print_error(f"Verification failed: expected {mode}, got {regs[0] if regs else 'N/A'}")
    return False

def test_write_fan_speed(client, slave_id, speed, verbose=True):
    """Test writing fan speed"""
    fan_names = {FAN_AUTO: "AUTO", FAN_LOW: "LOW", FAN_MID: "MID", FAN_HIGH: "HIGH"}
    if verbose:
        print_header(f"TEST: Write Fan Speed ({fan_names.get(speed, 'UNKNOWN')})")
    
    if safe_write_register(client, slave_id, MB_REG_FAN_SPEED, speed):
        time.sleep(0.2)
        regs = safe_read_holding_registers(client, slave_id, MB_REG_FAN_SPEED, 1)
        if regs and regs[0] == speed:
            print_success(f"Fan speed set to {fan_names.get(speed, 'UNKNOWN')} (verified)")
            return True
        else:
            print_error(f"Verification failed: expected {speed}, got {regs[0] if regs else 'N/A'}")
    return False

def test_write_coils(client, slave_id, verbose=True):
    """Test writing coils (DND/MUR)"""
    if verbose:
        print_header("TEST: Write Coils (DND & MUR)")
    
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

# ── Test Modes ────────────────────────────────────────────────────────────────
def run_quick_test(client, slave_id):
    """Run quick single-cycle test"""
    print_header("Quick Test Mode - Single Cycle")
    
    test_read_holding_registers(client, slave_id)
    time.sleep(0.5)
    
    test_read_input_registers(client, slave_id)
    time.sleep(0.5)
    
    test_read_coils(client, slave_id)
    time.sleep(0.5)
    
    test_read_discrete_inputs(client, slave_id)
    time.sleep(0.5)
    
    test_write_target_temp(client, slave_id, 23.5)
    time.sleep(0.5)
    
    test_write_hvac_mode(client, slave_id, HVAC_HEAT)
    time.sleep(0.5)
    
    test_write_fan_speed(client, slave_id, FAN_MID)
    time.sleep(0.5)
    
    test_write_coils(client, slave_id)
    time.sleep(0.5)

    test_write_relay_mode(client, slave_id, RELAY_3SPEED)
    time.sleep(0.5)

    print_header("Quick test completed successfully!")

def run_cycle_test(client, slave_id, cycles):
    """Run N-cycle automated test"""
    print_header(f"Automated Test Mode - {cycles} Cycles (Full Test)")
    
    for cycle in range(1, cycles + 1):
        print(f"\n{Fore.MAGENTA}{'=' * 70}")
        print(f"{Fore.MAGENTA}Test Cycle #{cycle}/{cycles}")
        print(f"{Fore.MAGENTA}{'=' * 70}{Style.RESET_ALL}")
        
        # Read all holding registers
        test_read_holding_registers(client, slave_id, verbose=True)
        time.sleep(0.5)
        
        # Read all input registers
        test_read_input_registers(client, slave_id, verbose=True)
        time.sleep(0.5)
        
        # Read coils
        test_read_coils(client, slave_id, verbose=True)
        time.sleep(0.5)
        
        # Read discrete inputs
        test_read_discrete_inputs(client, slave_id, verbose=True)
        time.sleep(0.5)
        
        # Cycle through target temperatures
        temps = [20.0, 22.0, 24.0, 21.5, 23.0, 19.0, 25.0]
        test_write_target_temp(client, slave_id, temps[cycle % len(temps)])
        time.sleep(0.5)
        
        # Cycle through HVAC modes
        modes = [HVAC_OFF, HVAC_HEAT, HVAC_COOL]
        test_write_hvac_mode(client, slave_id, modes[cycle % len(modes)])
        time.sleep(0.5)
        
        # Cycle through fan speeds
        speeds = [FAN_AUTO, FAN_LOW, FAN_MID, FAN_HIGH]
        test_write_fan_speed(client, slave_id, speeds[cycle % len(speeds)])
        time.sleep(0.5)
        
        # Test coil writes (DND/MUR ON/OFF)
        test_write_coils(client, slave_id)
        time.sleep(0.5)

        # Cycle through relay modes
        relay_modes = [RELAY_3SPEED, RELAY_1RELAY]
        test_write_relay_mode(client, slave_id, relay_modes[cycle % len(relay_modes)])
        time.sleep(0.5)

        print_success(f"Cycle {cycle}/{cycles} completed successfully!")
        if cycle < cycles:
            print_info("Pausing 1 second before next cycle...")
            time.sleep(1.0)
    
    print(f"\n{Fore.CYAN}{'=' * 70}")
    print(f"{Fore.CYAN}All {cycles} test cycles completed!")
    print(f"{Fore.CYAN}{'=' * 70}{Style.RESET_ALL}")

def run_continuous_test(client, slave_id, interval=2.0):
    """Run continuous test loop"""
    print_info(f"Starting continuous test loop (press Ctrl+C to stop)")
    print_info(f"Test interval: {interval} seconds\n")
    
    test_count = 0
    try:
        while True:
            test_count += 1
            print(f"\n{Fore.MAGENTA}{'=' * 70}")
            print(f"{Fore.MAGENTA}Test Cycle #{test_count}")
            print(f"{Fore.MAGENTA}{'=' * 70}{Style.RESET_ALL}")
            
            test_read_holding_registers(client, slave_id)
            time.sleep(0.5)
            
            test_read_input_registers(client, slave_id)
            time.sleep(0.5)
            
            test_read_coils(client, slave_id)
            time.sleep(0.5)
            
            test_read_discrete_inputs(client, slave_id)
            time.sleep(0.5)
            
            # Cycle through values
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

            # Cycle through relay modes
            relay_modes = [RELAY_3SPEED, RELAY_1RELAY]
            test_write_relay_mode(client, slave_id, relay_modes[test_count % len(relay_modes)])
            time.sleep(0.5)

            print_info(f"Waiting {interval} seconds before next cycle...")
            time.sleep(interval)
            
    except KeyboardInterrupt:
        print(f"\n{Fore.YELLOW}Test stopped by user.{Style.RESET_ALL}")

# ── Main ──────────────────────────────────────────────────────────────────────
def main():
    """Main entry point"""
    parser = argparse.ArgumentParser(
        description='Hotel Room Thermostat - Modbus RTU Test Suite',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  %(prog)s                          Interactive menu
  %(prog)s --quick                   Quick single test (auto-detect)
  %(prog)s --cycles 5                Run 5 automated test cycles
  %(prog)s --continuous              Continuous test loop
  %(prog)s --port COM6 --slave 2 --quick
        """
    )
    
    parser.add_argument('--port', default='COM6', help='COM port (default: COM6)')
    parser.add_argument('--slave', type=int, default=1, help='Modbus slave address (default: 1)')
    parser.add_argument('--baudrate', type=int, default=115200, help='Baud rate (default: 115200)')
    parser.add_argument('--quick', action='store_true', help='Quick single-cycle test')
    parser.add_argument('--cycles', type=int, metavar='N', help='Run N automated test cycles')
    parser.add_argument('--continuous', action='store_true', help='Continuous test loop')
    parser.add_argument('--interval', type=float, default=2.0, help='Interval for continuous mode (seconds)')
    
    args = parser.parse_args()
    
    # Determine test mode
    if not (args.quick or args.cycles or args.continuous):
        # Interactive menu
        print_header("Hotel Room Thermostat - Modbus RTU Test Suite")
        print(f"\n{Fore.CYAN}Select test mode:{Style.RESET_ALL}")
        print(f"  1. Quick test (single cycle, all functions)")
        print(f"  2. Automated test (multiple cycles)")
        print(f"  3. Continuous test (loop until Ctrl+C)")
        print(f"  4. Exit")
        
        try:
            choice = input(f"\n{Fore.CYAN}Enter choice (1-4): {Style.RESET_ALL}").strip()
            
            if choice == '1':
                args.quick = True
            elif choice == '2':
                cycles = int(input(f"{Fore.CYAN}Enter number of cycles: {Style.RESET_ALL}"))
                args.cycles = cycles
            elif choice == '3':
                args.continuous = True
                interval = input(f"{Fore.CYAN}Enter interval in seconds (default 2.0): {Style.RESET_ALL}")
                args.interval = float(interval) if interval else 2.0
            elif choice == '4':
                print_info("Exiting...")
                return 0
            else:
                print_error("Invalid choice")
                return 1
                
            # Get port and slave ID
            port_input = input(f"{Fore.CYAN}Enter COM port (default COM6): {Style.RESET_ALL}").strip()
            args.port = port_input if port_input else 'COM6'
            
            slave_input = input(f"{Fore.CYAN}Enter Modbus slave address (default 1): {Style.RESET_ALL}").strip()
            args.slave = int(slave_input) if slave_input else 1
            
        except (ValueError, KeyboardInterrupt) as e:
            print(f"\n{Fore.YELLOW}Cancelled by user.{Style.RESET_ALL}")
            return 0
    
    # Validate slave address
    if not (1 <= args.slave <= 247):
        print_error("Slave address must be between 1 and 247")
        return 1
    
    # Connect to Modbus device
    print_info(f"Connecting to {args.port} at {args.baudrate} baud, slave ID {args.slave}...")
    
    client = ModbusSerialClient(
        port=args.port,
        baudrate=args.baudrate,
        bytesize=8,
        parity='N',
        stopbits=1,
        timeout=1.0
    )
    
    if not client.connect():
        print_error(f"Failed to connect to {args.port}")
        return 1
    
    print_success(f"Connected to {args.port}\n")
    
    try:
        # Run selected test mode
        if args.quick:
            run_quick_test(client, args.slave)
        elif args.cycles:
            run_cycle_test(client, args.slave, args.cycles)
        elif args.continuous:
            run_continuous_test(client, args.slave, args.interval)
            
    finally:
        client.close()
        print_info("Connection closed")
    
    return 0

if __name__ == "__main__":
    sys.exit(main())
