from PyQt5.QtWidgets import QApplication, QMainWindow, QLabel, QVBoxLayout, QWidget, QPushButton
from PyQt5.QtCore import QTimer
import serial

class MainWindow(QMainWindow):
    def __init__(self):
        super().__init__()
        self.setWindowTitle("Interfaz del Coche")
        self.setGeometry(100, 100, 600, 400)

        # Configura el puerto serial
        self.ser = serial.Serial('COM3', 9600, timeout=1)

        # Configura la interfaz
        layout = QVBoxLayout()
        
        self.label_revolutions = QLabel("Revoluciones: 0")
        self.label_speed = QLabel("Velocidad: 0")
        self.label_temp = QLabel("Temperatura: 0")
        self.label_fuel = QLabel("Consumo: 0")

        layout.addWidget(self.label_revolutions)
        layout.addWidget(self.label_speed)
        layout.addWidget(self.label_temp)
        layout.addWidget(self.label_fuel)

        # Agrega un botón para actualizar manualmente
        self.button_update = QPushButton("Actualizar")
        self.button_update.clicked.connect(self.update_data)
        layout.addWidget(self.button_update)

        container = QWidget()
        container.setLayout(layout)
        self.setCentralWidget(container)

        # Configura un temporizador para actualización periódica
        self.timer = QTimer()
        self.timer.timeout.connect(self.update_data)
        self.timer.start(1000)  # Actualiza cada segundo

    def update_data(self):
        if self.ser.in_waiting > 0:
            try:
                data = self.ser.readline().decode('utf-8').strip()
                # Suponiendo que los datos llegan en un formato delimitado
                revs, speed, temp, fuel = data.split(',')
                self.label_revolutions.setText(f"Revoluciones: {revs}")
                self.label_speed.setText(f"Velocidad: {speed}")
                self.label_temp.setText(f"Temperatura: {temp}")
                self.label_fuel.setText(f"Consumo: {fuel}")
            except ValueError:
                print("Error en el formato de los datos")

if __name__ == '__main__':
    app = QApplication([])
    window = MainWindow()
    window.show()
    app.exec_()
