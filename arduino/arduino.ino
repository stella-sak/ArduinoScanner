#include <Servo.h>

const int TRIG_PIN = 9;
const int ECHO_PIN = 10;

const int GREEN_LED_PIN = 2;
const int YELLOW_LED_PIN = 3;
const int RED_LED_PIN = 4;
const int BUZZER_PIN = 5;

const int SERVO_PIN = 6;

const float WARNING_DISTANCE_CM = 30.0;
const float STOP_DISTANCE_CM = 15.0;

const int MIN_ANGLE = 15;
const int MAX_ANGLE = 165;
const int CENTER_ANGLE = 90;
const int ANGLE_STEP = 5;

const unsigned long SCAN_INTERVAL_MS = 180;
const unsigned long IDLE_MEASURE_INTERVAL_MS = 400;

Servo radarServo;

bool systemArmed = true;
bool emergencyStop = false;
bool scanning = false;

int currentAngle = CENTER_ANGLE;
int scanDirection = 1;

float lastDistanceCm = -1.0;
float closestDistanceCm = -1.0;
int closestAngleDeg = -1;

String currentStatus = "STARTING";

unsigned long lastScanTime = 0;
unsigned long lastIdleMeasureTime = 0;

void setup() {
    Serial.begin(9600);

    pinMode(TRIG_PIN, OUTPUT);
    pinMode(ECHO_PIN, INPUT);

    pinMode(GREEN_LED_PIN, OUTPUT);
    pinMode(YELLOW_LED_PIN, OUTPUT);
    pinMode(RED_LED_PIN, OUTPUT);
    pinMode(BUZZER_PIN, OUTPUT);

    digitalWrite(TRIG_PIN, LOW);
    allOutputsOff();

    radarServo.attach(SERVO_PIN);
    radarServo.write(CENTER_ANGLE);

    delay(1000);

    Serial.println("{\"message\":\"Radar ready\"}");
}

void loop() {
  readCommandsFromSerial();

  if (emergencyStop) {
    currentStatus = "EMERGENCY_STOP";
    updateOutputs(currentStatus);
    sendTelemetry();
    delay(250);
    return;
  }

  unsigned long now = millis();

  if (scanning) {
    if (now - lastScanTime >= SCAN_INTERVAL_MS) {
      lastScanTime = now;
      performScanStep();
    }
  } else {
    if (now - lastIdleMeasureTime >= IDLE_MEASURE_INTERVAL_MS) {
      lastIdleMeasureTime = now;
      measureAtCurrentAngle();
    }
  }
}


void readCommandsFromSerial() {
  if (Serial.available() > 0) {
    String command = Serial.readStringUntil('\n');
    command.trim();
    command.toUpperCase();

    if (command == "START_SCAN") {
      systemArmed = true;
      emergencyStop = false;
      scanning = true;
      resetClosestObject();
    } else if (command == "STOP_SCAN") {
      scanning = false;
    } else if (command == "CENTER") {
      scanning = false;
      currentAngle = CENTER_ANGLE;
      radarServo.write(currentAngle);
      delay(250);
      measureAtCurrentAngle();
    } else if (command == "ARM") {
      systemArmed = true;
      emergencyStop = false;
    } else if (command == "DISARM") {
      systemArmed = false;
      emergencyStop = false;
      scanning = false;
      allOutputsOff();
    } else if (command == "EMERGENCY_STOP") {
      systemArmed = true;
      emergencyStop = true;
      scanning = false;
    } else if (command == "RESET") {
      systemArmed = true;
      emergencyStop = false;
      scanning = false;
      currentAngle = CENTER_ANGLE;
      scanDirection = 1;
      resetClosestObject();
      radarServo.write(currentAngle);
      delay(250);
      measureAtCurrentAngle();
    }
  }
}

void performScanStep() {
  radarServo.write(currentAngle);

  // Give the servo a little time to physically move.
  delay(70);

  lastDistanceCm = readDistanceCm();
  currentStatus = decideStatus(lastDistanceCm);

  updateClosestObject(lastDistanceCm, currentAngle);
  updateOutputs(currentStatus);
  sendTelemetry();

  currentAngle = currentAngle + (scanDirection * ANGLE_STEP);

  if (currentAngle >= MAX_ANGLE) {
    currentAngle = MAX_ANGLE;
    scanDirection = -1;
  } else if (currentAngle <= MIN_ANGLE) {
    currentAngle = MIN_ANGLE;
    scanDirection = 1;
  }
}

void measureAtCurrentAngle() {
  radarServo.write(currentAngle);
  delay(70);

  lastDistanceCm = readDistanceCm();
  currentStatus = decideStatus(lastDistanceCm);

  updateClosestObject(lastDistanceCm, currentAngle);
  updateOutputs(currentStatus);
  sendTelemetry();
}

float readDistanceCm() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);

  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  unsigned long durationMicroseconds = pulseIn(ECHO_PIN, HIGH, 30000);

  if (durationMicroseconds == 0) {
    return -1.0;
  }

  float distanceCm = durationMicroseconds * 0.0343 / 2.0;
  return distanceCm;
}

String decideStatus(float distanceCm) {
  if (!systemArmed) {
    return "DISARMED";
  }

  if (emergencyStop) {
    return "EMERGENCY_STOP";
  }

  if (distanceCm < 0) {
    return "NO_READING";
  }

  if (distanceCm <= STOP_DISTANCE_CM) {
    return "STOP";
  }

  if (distanceCm <= WARNING_DISTANCE_CM) {
    return "WARNING";
  }

  return "SAFE";
}

void resetClosestObject() {
  closestDistanceCm = -1.0;
  closestAngleDeg = -1;
}

void updateClosestObject(float distanceCm, int angleDeg) {
  if (distanceCm < 0) {
    return;
  }

  // Ignore very far/noisy values for the closest-object display.
  if (distanceCm > 200.0) {
    return;
  }

  if (closestDistanceCm < 0 || distanceCm < closestDistanceCm) {
    closestDistanceCm = distanceCm;
    closestAngleDeg = angleDeg;
  }
}


void allOutputsOff() {
  digitalWrite(GREEN_LED_PIN, LOW);
  digitalWrite(YELLOW_LED_PIN, LOW);
  digitalWrite(RED_LED_PIN, LOW);
  noTone(BUZZER_PIN);
}

void updateOutputs(String status) {
  allOutputsOff();

  if (status == "SAFE") {
    digitalWrite(GREEN_LED_PIN, HIGH);
  } else if (status == "WARNING") {
    digitalWrite(YELLOW_LED_PIN, HIGH);
  } else if (status == "STOP") {
    digitalWrite(RED_LED_PIN, HIGH);
    tone(BUZZER_PIN, 1000);
  } else if (status == "EMERGENCY_STOP") {
    digitalWrite(RED_LED_PIN, HIGH);
    tone(BUZZER_PIN, 1500);
  } else if (status == "NO_READING") {
    digitalWrite(YELLOW_LED_PIN, HIGH);
  }

}

void sendTelemetry() {
  Serial.print("{");

  Serial.print("\"angle_deg\":");
  Serial.print(currentAngle);

  Serial.print(",\"distance_cm\":");
  if (lastDistanceCm < 0) {
    Serial.print("null");
  } else {
    Serial.print(lastDistanceCm, 1);
  }

  Serial.print(",\"status\":\"");
  Serial.print(currentStatus);
  Serial.print("\"");

  Serial.print(",\"scanning\":");
  Serial.print(scanning ? "true" : "false");

  Serial.print(",\"armed\":");
  Serial.print(systemArmed ? "true" : "false");

  Serial.print(",\"emergency_stop\":");
  Serial.print(emergencyStop ? "true" : "false");

  Serial.print(",\"closest_distance_cm\":");
  if (closestDistanceCm < 0) {
    Serial.print("null");
  } else {
    Serial.print(closestDistanceCm, 1);
  }

  Serial.print(",\"closest_angle_deg\":");
  if (closestAngleDeg < 0) {
    Serial.print("null");
  } else {
    Serial.print(closestAngleDeg);
  }

  Serial.print(",\"green_led\":");
  Serial.print(digitalRead(GREEN_LED_PIN));

  Serial.print(",\"yellow_led\":");
  Serial.print(digitalRead(YELLOW_LED_PIN));

  Serial.print(",\"red_led\":");
  Serial.print(digitalRead(RED_LED_PIN));

  Serial.print(",\"buzzer\":");
  if (currentStatus == "STOP" || currentStatus == "EMERGENCY_STOP") {
    Serial.print(1);
  } else {
    Serial.print(0);
  }

  Serial.println("}");
}