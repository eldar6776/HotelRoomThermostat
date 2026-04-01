#!/usr/bin/env python3
"""Quick Modbus test runner without interactive input"""

import sys
import time
from modbus_test import *

def main():
    port = "COM6"
    slave_id = 1
    
    print_header("Quick Modbus Test - COM6 @ 115200 baud, Slave 1")
    print_info(f"Connecting to {port}...")
    
    client = ModbusSerialClient(
        port=port,
        baudrate=115200,
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
        # Run one cycle of all tests
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
        
        test_weather_data_write(client, slave_id)
        
        print_header("All tests completed successfully!")
        
    finally:
        client.close()
        print_info("Connection closed")

if __name__ == "__main__":
    main()
