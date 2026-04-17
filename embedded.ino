#include <SoftwareSerial.h>

SoftwareSerial BT(2, 3);

//Pins
int relay1    = 5;
int relay2    = 6;
int buzzer    = 8;
int ledStatus = 9;
int ledAlert  = 10;
int soundPin  = A0;

//States
int  systemState  = 0;
bool manualBuzzer = false;
bool alarmActive  = false;

//Sound config
const int   NUM_SAMPLES    = 5;
const int   SPIKE_MARGIN   = 50;
const float BASELINE_ALPHA = 0.95;

//Smart detection variables
int spikeCount = 0;
unsigned long lastSpikeTime = 0;
const int SPIKE_WINDOW = 2000; //2 sec

float baselineSound = 0;
bool  baselineReady = false;

//averaged analog read
int readSoundAvg() {
  long sum = 0;
  for (int i = 0; i < NUM_SAMPLES; i++) {
    sum += analogRead(soundPin);
    delayMicroseconds(200);
  }
  return sum / NUM_SAMPLES;
}

//callibration
void calibrateBaseline() {
  Serial.println("Calibrating... stay quiet.");
  long sum = 0;
  for (int i = 0; i < 50; i++) {
    sum += analogRead(soundPin);
    delay(20);
  }
  baselineSound = sum / 50.0;
  baselineReady = true;

  Serial.print("Baseline set to: ");
  Serial.println(baselineSound);
}

//command handler (bluetooth and GUI)
void processCommand(String data) {
  data.trim();
  data.toLowerCase();

  Serial.print("CMD: ");
  Serial.println(data);

  //light control
  if (data == "turn on light in living room") {
    digitalWrite(relay1, LOW);
  }
  else if (data == "turn off light in living room") {
    digitalWrite(relay1, HIGH);
  }
  else if (data == "turn on light in bedroom") {
    digitalWrite(relay2, LOW);
  }
  else if (data == "turn off light in bedroom") {
    digitalWrite(relay2, HIGH);
  }

  //security system
  else if (data == "enable security system") {
    systemState = 1;
    calibrateBaseline();
    digitalWrite(ledStatus, HIGH);
    Serial.println("ARMED");
  }
  else if (data == "disable security system") {
    systemState  = 0;
    alarmActive  = false;
    manualBuzzer = false;
    spikeCount   = 0;

    digitalWrite(ledStatus, LOW);
    digitalWrite(ledAlert,  LOW);
    noTone(buzzer);

    Serial.println("DISARMED");
  }

  //buzzer
  else if (data == "turn off buzzer") {
    alarmActive  = false;
    manualBuzzer = false;
    spikeCount   = 0;

    digitalWrite(ledAlert, LOW);
    noTone(buzzer);

    Serial.println("Buzzer stopped");
  }
}

void setup() {
  Serial.begin(9600);
  BT.begin(9600);

  pinMode(relay1,    OUTPUT);
  pinMode(relay2,    OUTPUT);
  pinMode(buzzer,    OUTPUT);
  pinMode(ledStatus, OUTPUT);
  pinMode(ledAlert,  OUTPUT);

  digitalWrite(relay1,    HIGH);
  digitalWrite(relay2,    HIGH);
  digitalWrite(ledStatus, LOW);
  digitalWrite(ledAlert,  LOW);
  noTone(buzzer);

  calibrateBaseline();
  Serial.println("System ready.");
}

void loop() {

  //read from bluetooth
  if (BT.available()) {
    String data = "";

    while (BT.available()) {
      char ch = BT.read();
      data += ch;
      delay(5);
    }

    processCommand(data);
  }

  //read from python GUI
  if (Serial.available()) {
    String data = Serial.readStringUntil('\n');
    processCommand(data);
  }

  //read sound
  int soundValue = readSoundAvg();

  // Update baseline only if no alarm
  if (!alarmActive) {
    baselineSound = BASELINE_ALPHA * baselineSound
                  + (1.0 - BASELINE_ALPHA) * soundValue;
  }

  int diff = soundValue - (int)baselineSound;

  //smart spike detection
  if (systemState == 1) {

    if (abs(diff) > SPIKE_MARGIN) {

      if (millis() - lastSpikeTime < SPIKE_WINDOW) {
        spikeCount++;
      } else {
        spikeCount = 1;
      }

      lastSpikeTime = millis();

      if (spikeCount >= 3 && !alarmActive) {
        alarmActive = true;
        Serial.println(" Suspicious sound pattern detected!");
      }
    }
  }

  //serial output for GUI
  Serial.print("SOUND:");
  Serial.print(soundValue);

  Serial.print(",BASE:");
  Serial.print((int)baselineSound);

  Serial.print(",ALARM:");
  Serial.print(alarmActive);

  Serial.print(",R1:");
  Serial.print(digitalRead(relay1));

  Serial.print(",R2:");
  Serial.print(digitalRead(relay2));

  Serial.print(",SYS:");
  Serial.println(systemState);

  //output control
  if (alarmActive) {
    digitalWrite(ledAlert, HIGH);
    tone(buzzer, 1200);
  } else {
    digitalWrite(ledAlert, LOW);
    if (manualBuzzer) tone(buzzer, 1000);
    else              noTone(buzzer);
  }

  delay(50);
}