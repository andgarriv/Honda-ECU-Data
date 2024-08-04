import tkinter as tk
from tkinter import ttk
import serial
import threading

class SerialReader(threading.Thread):
    def __init__(self, port, baudrate, callback):
        super().__init__()
        self.port = port
        self.baudrate = baudrate
        self.callback = callback
        self.running = True
        self.serial = serial.Serial(port, baudrate, timeout=1)

    def run(self):
        while self.running:
            if self.serial.in_waiting > 0:
                line = self.serial.readline().decode('utf-8').strip()
                self.callback(line)

    def stop(self):
        self.running = False
        self.serial.close()

class GUI(tk.Tk):
    def __init__(self):
        super().__init__()
        self.title("ECU Data Monitor")
        self.geometry("400x300")

        self.data_labels = {}
        self.create_widgets()
        self.serial_reader = SerialReader('COM5', 115200, self.update_data)
        self.serial_reader.start()

    def create_widgets(self):
        self.frame = ttk.Frame(self)
        self.frame.pack(fill=tk.BOTH, expand=True)

        self.data_display = ttk.Label(self.frame, text="Waiting for data...", font=("Arial", 12))
        self.data_display.pack(pady=20)

        self.quit_button = ttk.Button(self.frame, text="Quit", command=self.quit)
        self.quit_button.pack(pady=10)

    def update_data(self, data):
        lines = data.split(',')
        formatted_data = ""

        for line in lines:
            key_value = line.split(':')
            if len(key_value) == 2:
                key, value = key_value
                formatted_data += f"{key}: {value}\n"
        
        self.data_display.config(text=formatted_data)

    def quit(self):
        self.serial_reader.stop()
        self.destroy()

if __name__ == "__main__":
    app = GUI()
    app.mainloop()
