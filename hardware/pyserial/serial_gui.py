#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import tkinter as tk
from tkinter import ttk, scrolledtext, messagebox
import serial
import serial.tools.list_ports
import threading
import queue
import time


class SerialGUI(tk.Tk):
    def __init__(self):
        super().__init__()
        self.title("Serial Tool GUI")
        self.geometry("600x400")
        
        # 串口对象、读取线程及其状态
        self.serial_port = None
        self.read_thread = None
        self.running = False
        
        # 队列用于线程间传递接收到的数据
        self.queue = queue.Queue()
        
        # 复选框变量（True 为十六进制显示，否则为文本显示）
        self.display_as_hex = tk.BooleanVar(value=True)
        
        self.create_widgets()

    def create_widgets(self):
        # 顶部面板: 串口选择、波特率、连接按钮及显示模式切换
        top_frame = ttk.Frame(self)
        top_frame.pack(side=tk.TOP, fill=tk.X, padx=5, pady=5)
        
        ttk.Label(top_frame, text="Serial Port:").pack(side=tk.LEFT)
        self.port_combo = ttk.Combobox(top_frame, values=self.get_serial_ports(),
                                       state="readonly", width=15)
        self.port_combo.pack(side=tk.LEFT, padx=5)
        
        ttk.Label(top_frame, text="Baud Rate:").pack(side=tk.LEFT)
        self.baud_entry = ttk.Entry(top_frame, width=10)
        self.baud_entry.insert(0, "9600")
        self.baud_entry.pack(side=tk.LEFT, padx=5)
        
        self.connect_button = ttk.Button(top_frame, text="Connect", command=self.toggle_connection)
        self.connect_button.pack(side=tk.LEFT, padx=5)
        
        self.hex_checkbox = ttk.Checkbutton(top_frame, text="Display as Hex",
                                            variable=self.display_as_hex)
        self.hex_checkbox.pack(side=tk.LEFT, padx=5)
        
        # 中间区域：接收到的数据显示区（带滚动条）
        self.rx_text = scrolledtext.ScrolledText(self, wrap=tk.WORD, height=15, state="disabled")
        self.rx_text.pack(side=tk.TOP, fill=tk.BOTH, expand=True, padx=5, pady=5)
        
        # 底部面板：发送数据输入及发送按钮
        bottom_frame = ttk.Frame(self)
        bottom_frame.pack(side=tk.BOTTOM, fill=tk.X, padx=5, pady=5)
        
        self.send_entry = ttk.Entry(bottom_frame)
        self.send_entry.pack(side=tk.LEFT, fill=tk.X, expand=True, padx=5)
        
        send_button = ttk.Button(bottom_frame, text="Send", command=self.send_data)
        send_button.pack(side=tk.LEFT, padx=5)
        
        # 设置定时器，定期检查队列中是否有新数据
        self.after(100, self.process_queue)

    def get_serial_ports(self):
        ports = serial.tools.list_ports.comports()
        return [port.device for port in ports]

    def toggle_connection(self):
        if self.serial_port and self.serial_port.is_open:
            self.disconnect()
        else:
            self.connect()

    def connect(self):
        port = self.port_combo.get()
        try:
            baud_rate = int(self.baud_entry.get())
        except ValueError:
            messagebox.showerror("Error", "Invalid baud rate!")
            return

        if not port:
            messagebox.showerror("Error", "Please select a serial port.")
            return

        try:
            self.serial_port = serial.Serial(port, baud_rate, timeout=0.5)
            self.running = True

            # 启动后台线程收数据
            self.read_thread = threading.Thread(target=self.read_serial, daemon=True)
            self.read_thread.start()

            self.connect_button.config(text="Disconnect")
            self.port_combo.config(state="disabled")
            self.baud_entry.config(state="disabled")
        except Exception as e:
            messagebox.showerror("Error", f"Failed to open port: {e}")

    def disconnect(self):
        self.running = False
        if self.serial_port:
            self.serial_port.close()
        self.connect_button.config(text="Connect")
        self.port_combo.config(state="readonly")
        self.baud_entry.config(state="normal")

    def read_serial(self):
        while self.running and self.serial_port and self.serial_port.is_open:
            try:
                if self.serial_port.in_waiting:
                    data = self.serial_port.read(self.serial_port.in_waiting)
                    self.queue.put(data)
                time.sleep(0.05)
            except Exception as e:
                self.queue.put(f"Error reading from serial: {e}".encode())
                break

    def process_queue(self):
        # 从队列中取数据并更新显示区域
        while not self.queue.empty():
            data = self.queue.get()
            if isinstance(data, bytes):
                if self.display_as_hex.get():
                    text = ' '.join(f"{b:02X}" for b in data)
                else:
                    try:
                        text = data.decode('utf-8', errors='replace')
                    except Exception:
                        text = str(data)
            else:
                text = str(data)
            self.rx_text.config(state="normal")
            self.rx_text.insert(tk.END, text + "\n")
            self.rx_text.yview(tk.END)
            self.rx_text.config(state="disabled")
        self.after(100, self.process_queue)

    def send_data(self):
        if self.serial_port and self.serial_port.is_open:
            user_input = self.send_entry.get().strip()
            if not user_input:
                return
            try:
                # 尝试按空格分隔解析每段为十六进制数字
                parts = user_input.split()
                data = bytearray(int(p, 16) for p in parts)
            except Exception:
                # 若解析失败，则按文本发送
                data = user_input.encode()
            try:
                self.serial_port.write(data)
                # 将发送的数据以十六进制形式显示到接收窗口中
                self.rx_text.config(state="normal")
                self.rx_text.insert(tk.END, "[TX] " + ' '.join(f"{b:02X}" for b in data) + "\n")
                self.rx_text.yview(tk.END)
                self.rx_text.config(state="disabled")
            except Exception as e:
                messagebox.showerror("Error", f"Error sending data: {e}")
            self.send_entry.delete(0, tk.END)
        else:
            messagebox.showwarning("Warning", "Serial port is not connected.")


if __name__ == "__main__":
    app = SerialGUI()
    app.mainloop()
