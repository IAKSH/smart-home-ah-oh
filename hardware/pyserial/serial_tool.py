#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import serial
import serial.tools.list_ports
import threading
import sys

# 全局变量，用于控制接收数据的显示模式： "hex" 或 "text"
rx_display_mode = "hex"

def list_serial_ports():
    """
    Detect and display all available serial port devices.
    """
    ports = list(serial.tools.list_ports.comports())
    if not ports:
        print("No serial ports detected!")
        sys.exit(0)
    print("Available serial ports:")
    for i, port in enumerate(ports):
        print(f"[{i}] {port.device} - {port.description}")
    return ports

def to_hex_str(data):
    """
    Convert byte data to a hexadecimal string representation.
    """
    return ' '.join(f"{b:02X}" for b in data)

def serial_receive(ser):
    """
    Run in a separate thread: continuously read from the serial port
    and display the data based on the current display mode.
    """
    global rx_display_mode
    while True:
        try:
            if ser.in_waiting:
                data = ser.read(ser.in_waiting)
                if data:
                    # Use a new line to prevent overlapping with user input prompt.
                    if rx_display_mode == "hex":
                        print("\n[RX] " + to_hex_str(data))
                    else:
                        try:
                            # Decode as UTF-8; if decoding fails, replace errors.
                            text = data.decode('utf-8', errors='replace')
                        except Exception:
                            text = str(data)
                        print("\n[RX] " + text)
        except Exception as e:
            print("Error reading from serial port:", e)
            break

def main():
    global rx_display_mode

    # List all available serial ports.
    ports = list_serial_ports()
    selection = input("Select serial port index: ")
    try:
        selection = int(selection)
        selected_port = ports[selection].device
    except Exception:
        print("Invalid selection")
        sys.exit(1)
    
    # Input baud rate.
    baud_rate = input("Enter baud rate (e.g., 9600, 115200): ")
    try:
        baud_rate = int(baud_rate)
    except Exception:
        print("Invalid baud rate input, defaulting to 9600")
        baud_rate = 9600

    # Open the serial port.
    try:
        ser = serial.Serial(selected_port, baudrate=baud_rate, timeout=0.5)
        print(f"Successfully opened {selected_port} at {baud_rate}bps")
    except Exception as e:
        print("Error opening serial port:", e)
        sys.exit(1)

    # Start the reception thread.
    recv_thread = threading.Thread(target=serial_receive, args=(ser,), daemon=True)
    recv_thread.start()

    print("Enter the data to send. Just press Enter on an empty line to exit.")
    print("If the input consists of space-separated hexadecimal numbers (e.g., 0x12 0xA5 or 12 A5),")
    print("it will be sent as hex; otherwise, it will be sent as text.")
    print("Type /toggle to switch the received data display mode between hexadecimal and text.")
    
    try:
        while True:
            user_input = input(">> ")
            if user_input == "":
                print("Exiting...")
                break

            # 命令 /toggle 用于切换显示模式
            if user_input.strip().lower() == "/toggle":
                if rx_display_mode == "hex":
                    rx_display_mode = "text"
                    print("Received data display mode switched to TEXT.")
                else:
                    rx_display_mode = "hex"
                    print("Received data display mode switched to HEX.")
                continue

            # 尝试解析为十六进制数据，如果失败则当作文本发送
            try:
                parts = user_input.split()
                data = bytearray(int(p, 16) for p in parts)
            except Exception:
                data = user_input.encode()

            # Send data.
            ser.write(data)
            print("[TX] " + to_hex_str(data))
    except KeyboardInterrupt:
        print("\nProgram interrupted by user.")
    
    ser.close()
    print("Serial port closed.")

if __name__ == "__main__":
    main()
