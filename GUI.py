import tkinter as tk
from tkinter import ttk, messagebox
import threading
import serial
from datetime import datetime

# Initialize variables
data_values = {
    "rpm": 0,
    "tps": 0,
    "speed": 0,
    "iat": 0,
    "ect": 0,
    "o2": 0,
    "voltage": 0.0,
    "stf": 0,
    "ltf": 0,
    "imap": 0.0,
    "ta": 0,
    "distance": 0,
    "consumption": 0,
    "time": 0
}

class SerialReader(threading.Thread):
    def __init__(self, port, baudrate, callback):
        super().__init__()
        self.port = port
        self.baudrate = baudrate
        self.callback = callback
        self.running = True
        try:
            self.serial = serial.Serial(port, baudrate, timeout=1)
        except serial.SerialException as e:
            self.running = False
            self.callback(f"Error: {e}")

    def run(self):
        while self.running:
            if self.serial.in_waiting > 0:
                line = self.serial.readline().decode('utf-8', errors='replace').strip()
                self.callback(line)

    def stop(self):
        self.running = False
        self.serial.close()

class GUI(tk.Tk):
    def __init__(self):
        super().__init__()
        self.title("Honda ECU Data Monitor")
        self.geometry("1280x720")
        
        self.start_time = None
        self.total_distance = 0.0

        self.configure(bg='#2C2C2C')
        self.style = ttk.Style()
        self.style.configure('TLabel', background='#2C2C2C', foreground='white')
        self.style.configure('TFrame', background='#2C2C2C')
        self.style.configure('TProgressbar', background='#2C2C2C')

        self.serial_reader = None
        self.labels = {}
        self.data_buffer = []

        self.create_widgets()

    def create_widgets(self):
        self.frame = ttk.Frame(self)
        self.frame.pack(fill=tk.BOTH, expand=True)

        self.create_data_display("RPM", 0, "", 0, 0)
        self.create_data_display("Speed", 0, "km/h", 0, 1)
        self.create_data_display("Voltage", 0.0, "V", 0, 2)
        
        self.create_data_display("TPS", 0, "%", 1, 0)
        self.create_data_display("IAT", 0, "°C", 1, 1)
        self.create_data_display("ECT", 0, "°C", 1, 2)
        
        self.create_data_display("MAP", 0.0, "kPa", 2, 0)
        self.create_data_display("Short Term Fuel", 0, "%", 2, 1)
        self.create_data_display("Long Term Fuel", 0, "%", 2, 2)
        
        self.create_data_display("Timing Advance", 0, "°", 3, 0)
        self.create_data_display("O2", 0, "V", 3, 1)
        self.create_data_display("Distance", 0, "km", 3, 2)

        self.connect_button = tk.Button(self.frame, text="Connect", command=self.connect_serial, bg='#161616', fg='white')
        self.connect_button.grid(row=8, column=0, columnspan=3, pady=10)


        # Adjust row and column weights to make them expand equally
        for i in range(10):  # Adjusting for the number of rows created
            self.frame.grid_rowconfigure(i, weight=1)
        for i in range(3):
            self.frame.grid_columnconfigure(i, weight=1)

    def create_rpm_display(self, value, max_value, row, col):
        # RPM specific design
        ttk.Label(self.frame, text="RPM", font=("Arial", 12, "bold")).grid(row=row * 2, column=col, padx=10, pady=5, sticky='nsew')
        self.rpm_value_label = ttk.Label(self.frame, text=str(value), font=("Arial", 12))
        self.rpm_value_label.grid(row=row * 2 + 1, column=col, padx=10, pady=5, sticky='nsew')
        self.rpm_progress = ttk.Progressbar(self.frame, orient="horizontal", length=200, mode="determinate", maximum=max_value)
        self.rpm_progress.grid(row=row * 2 + 2, column=col, padx=10, pady=5, sticky='nsew')
        self.rpm_progress['value'] = value

    def create_data_display(self, label_text, value, unit, row, col):
        # General design for other labels
        ttk.Label(self.frame, text=label_text, font=("Arial", 12, "bold")).grid(row=row * 2, column=col, padx=10, pady=5, sticky='nsew')
        label = ttk.Label(self.frame, text=f"{value} {unit}", font=("Arial", 12))
        label.grid(row=row * 2 + 1, column=col, padx=10, pady=5, sticky='nsew')
        self.labels[label_text.lower().replace(" ", "_")] = label

    def connect_serial(self):
        self.serial_reader = SerialReader('COM5', 115200, self.update_data)
        if self.serial_reader.running:
            self.start_time = datetime.now()
            self.serial_reader.start()
        else:
            self.show_error("Unable to connect to the specified COM port.")
                    
    def update_data(self, data):
        if data.startswith("Error:"):
            self.show_error(data)
            return
        
        try:
            # Assuming data is in the format: "Data,RPM: 0,ECT: 69,Voltage: 12.34,..."
            if data.startswith("Data,"):
                data = data[5:]  # Remove the "Data," prefix
            data_pairs = data.split(',')
            data_entry = {}
            for pair in data_pairs:
                key, value = pair.split(':')
                key = key.strip().lower().replace(" ", "_")
                value = value.strip()
                if key == "stf":
                    key = "short_term_fuel"
                elif key == "ltf":
                    key = "long_term_fuel"
                elif key == "ta":
                    key = "timing_advance"
                if key == "imap":
                    key = "map"

                if key in self.labels:
                    self.labels[key].config(text=f"{value} {self.get_unit(key)}")
                    data_entry[key] = value

            # Add the data to the buffer
            if data_entry:
                self.data_buffer.append(data_entry)
                
                # Update specific widgets like RPM progress bar
                if "rpm" in data_entry:
                    self.rpm_value_label.config(text=data_entry["rpm"])
                    self.rpm_progress['value'] = float(data_entry["rpm"])
                    
                # Update distance
                if "speed" in data_entry:
                    self.update_distance(float(data_entry["speed"]))
                    
        except (ValueError, KeyError):
            return
        
    def update_distance(self, speed):
        if self.start_time:
            elapsed_time = datetime.now() - self.start_time
            hours = elapsed_time.total_seconds() / 3600
            distance_increment = speed * hours
            self.total_distance += distance_increment
            self.labels["distance"].config(text=f"{self.total_distance:.2f} km")
            self.start_time = datetime.now()  # Reset the start time for the next interval


    def get_unit(self, key):
        units = {
            "rpm": "",
            "speed": "km/h",
            "voltage": "V",
            "tps": "%",
            "iat": "°C",
            "ect": "°C",
            "map": "kPa",
            "short_term_fuel": "%",
            "long_term_fuel": "%",
            "timing_advance": "°",
            "o2": "V",
            "distance": "km"
        }
        return units.get(key, "")

    def show_error(self, message):
        messagebox.showerror("Error", message)

if __name__ == "__main__":
    app = GUI()
    app.mainloop()
