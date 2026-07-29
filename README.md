# **Οδηγός Εγκατάστασης και Εκτέλεσης**

## **Απαραίτητος Εξοπλισμός**

- Arduino UNO R3
- HC-SR04 ultrasonic sensor
- Servo motor (SG90 MicroServo)
- Green, yellow, red LEDs
- Buzzer
- 3 Resistors 220Ω
- Jumper wires
- Breadboard
- USB cables




## **Συνδεσμολογία**

Arduino pins που θα χρειαστούμε: 5V, GND, D2, D3, D4, D5, D6, D9, D10.

### <ins>Sensor:</ins>
- Sensor TRIG -> D9
- Sensor ECHO -> D10
- Sensor VCC -> 5V
- Sensor GND -> GND

### <ins>Servo:</ins>
- Servo signal -> D6
- Servo VCC -> 5V
- Servo GND -> GND

### <ins>LEDs:</ins>
- LED short legs -> GND
- Green LED long leg -> D2 μέσω αντιστάτη
- Yellow LED long leg-> D3 μέσω αντιστάτη
- Red  LED long leg -> D4 μέσω αντιστάτη

### <ins>Buzzer:</ins>
- Positive -> D5
- Negative -> GND 



## **Υλοποίηση**
1. Ανοίξτε το αρχείο arduino/arduino.ino στο Arduino IDE.
2. Επιλέξτε το Arduino UNO board και το κατάλληλο COM port.
3. Κάντε upload στο Arduino.
4. Κλείστε το Serial Monitor.
5. Ανοίξτε το app.py αρχείο και αλλάξτε αν χρειάζεται το COM port: SERIAL_PORT = "COM3".
6. Ενεργοποιήστε το virtual environment στη python με τις εξής εντολές στo terminal:
   python -m venv .venv
   .\.venv\Scripts\Activate.ps1
7. Κατεβάστε τα απαραίτητα Python packages:
   pip install -r requirements.txt
8. Ανοίξτε τον server:
   python app.py
9. Μεταβείτε στη διεύθυνση: http://127.0.0.1:5000

