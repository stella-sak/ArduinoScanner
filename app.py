from datetime import datetime

import json
import threading
import time

from flask import Flask, jsonify, render_template, request

try:
    import serial
except ImportError:
    serial = None

app = Flask(__name__)

SERIAL_PORT = "COM3"
BAUD_RATE = 9600

VALID_COMMANDS = {
    "START_SCAN",
    "STOP_SCAN",
    "CENTER",
    "ARM",
    "DISARM",
    "EMERGENCY_STOP",
    "RESET",
}

state_lock = threading.Lock()
serial_write_lock = threading.Lock()
arduino_connection = None

radar_state = {
    "connection": "STARTING",
    "message": "Flask app started. Waiting for Arduino...",
    "angle_deg": None,
    "distance_cm": None,
    "status": "UNKNOWN",
    "scanning": False,
    "armed": None,
    "emergency_stop": None,
    "closest_distance_cm": None,
    "closest_angle_deg": None,
    "green_led": 0,
    "yellow_led": 0,
    "red_led": 0,
    "buzzer": 0,
    "last_update": None,
    "last_command": None,
    "raw_line": None,
}

    
def update_state(new_values):
    with state_lock:
        radar_state.update(new_values)


def get_state_copy():
    with state_lock:
        return dict(radar_state)



def serial_reader_loop():
    
    global arduino_connection

    if serial is None:
        update_state(
            {
                "connection": "ERROR",
                "message": "pySerial is not installed. Run:pip install pyserial",
            }
        )
    while True:
        try:
            update_state(
                {
                    "connection": "CONNECTING",
                    "message": f"Trying to connect to Arduino on {SERIAL_PORT}...",
                }
            )  

            with serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=1) as arduino:
                with serial_write_lock:
                    arduino_connection = arduino

                time.sleep(2)
                arduino.reset_input_buffer()
                
                update_state(
                    {
                        "connection": "CONNECTED",
                        "message": f"Connected to Arduino on {SERIAL_PORT}.",
                    }
                )

                while True:
                    raw_line = arduino.readline()
                    line = raw_line.decode("utf-8", errors="replace").strip()

                    if not line:
                        continue
                    
                    try:
                        data = json.loads(line)
                    except json.JSONDecodeError:
                        update_state(
                            {
                                "raw_line": line,
                                "message": "Received a line that was not valid JSON.",
                                "last_update": datetime.now().strftime("%H:%M:%S"),
                            }
                        )
                        continue

                    if "message" in data:
                            update_state(
                                {
                                    "connection": "CONNECTED",
                                    "message": data["message"],
                                    "raw_line": line,
                                    "last_update": datetime.now().strftime("%H:%M:%S"),
                                }
                            )
                            continue

                    update_state(
                        {
                            "connection": "CONNECTED",
                            "angle_deg": data.get("angle_deg"),
                            "distance_cm": data.get("distance_cm"),
                            "status": data.get("status", "UNKNOWN"),
                            "scanning": data.get("scanning", False),
                            "armed": data.get("armed"),
                            "emergency_stop": data.get("emergency_stop"),
                            "closest_distance_cm": data.get("closest_distance_cm"),
                            "closest_angle_deg": data.get("closest_angle_deg"),
                            "green_led": data.get("green_led", 0),
                            "yellow_led": data.get("yellow_led", 0),
                            "red_led": data.get("red_led", 0),
                            "buzzer": data.get("buzzer", 0),
                            "raw_line": line,
                            "last_update": datetime.now().strftime("%H:%M:%S"),
                        }
                    )
        except serial.SerialException as exc:
            
            with serial_write_lock:
                arduino_connection = None

            update_state(
                {
                    "connection": "DISCONNECTED",
                    "message": f"Serial error: {exc}",
                }
            )
            time.sleep(2)
        
        except Exception as exc:
            with serial_write_lock:
                arduino_connection = None

            update_state(
                {
                    "connection": "ERROR",
                    "message": f"Unexpected error: {exc}",
                }
            )
            time.sleep(2)

def send_command_to_arduino(command):
    global arduino_connection
    
    with serial_write_lock:
        if arduino_connection is None:
            return False, "Arduino is not connected"

        try:
            arduino_connection.write(f"{command}\n".encode("utf-8"))
            arduino_connection.flush()
            return True, f"Sent command: {command}"
        except serial.SerialException as exc:
            arduino_connection = None
            return False, f"Serial write error: {exc}"

@app.route("/")
def home():
    return render_template("index.html")

@app.route("/api/telemetry")
def telemetry():

    return jsonify(get_state_copy())

@app.route("/api/command", methods=["POST"])
def command():
    data = request.get_json()
    if not data or "command" not in data:
        return jsonify({"ok": False, "error": "Missing command"}), 400

    command_value = str(data["command"]).strip().upper()

    if command_value not in VALID_COMMANDS:
        return jsonify({"ok": False, "error": "Invalid command"}), 400

    ok, message = send_command_to_arduino(command_value)

    update_state(
        {
            "last_command": command_value,
            "message": message,
        }
    )

    if not ok:
        return jsonify({"ok": False, "error": message, "state": get_state_copy()}), 503

    return jsonify({"ok": True, "message": message, "state": get_state_copy()})

if __name__ == "__main__":

    reader_thread = threading.Thread(target=serial_reader_loop, daemon=True)
    reader_thread.start()

    app.run(debug=True, use_reloader=False)
    