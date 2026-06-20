#include <Bluepad32.h>

// =========================
// PINS
// =========================
#define LEFT_RPWM   18
#define LEFT_LPWM   19

#define RIGHT_RPWM  26
#define RIGHT_LPWM  25

#define ARM_LED     2      // ON = Robot Enabled
#define BT_LED      22     // ON = Controller Connected

#define BT_REPAIR_BTN 32   // Button -> GND
#define ARM_BTN       33   // Button -> GND

// =========================
// PWM SETTINGS
// =========================
const int PWM_FREQUENCY = 20000;
const int PWM_RESO      = 8;

#define CH_LEFT_RPWM  0
#define CH_LEFT_LPWM  1
#define CH_RIGHT_RPWM 2
#define CH_RIGHT_LPWM 3

ControllerPtr myController = nullptr;

// =========================
// GLOBAL VARIABLES
// =========================
bool robotEnabled = false;

bool lastArmBtnState = HIGH;
bool lastRepairBtnState = HIGH;

// =========================
// FORWARD DECLARATIONS
// =========================
void stopMotors();

// =========================
// CONTROLLER CALLBACKS
// =========================
void onConnectedController(ControllerPtr ctl) {
Serial.println("Controller Connected");

 
myController = ctl;

digitalWrite(BT_LED, HIGH);
 

}

void onDisconnectedController(ControllerPtr ctl) {
Serial.println("Controller Disconnected");

 
if (myController == ctl)
    myController = nullptr;

digitalWrite(BT_LED, LOW);

stopMotors();
 

}

// =========================
// MOTOR FUNCTIONS
// =========================
void setLeftMotor(int speed) {
speed = constrain(speed, -255, 255);

 
if (speed > 0) {
    ledcWrite(CH_LEFT_RPWM, speed);
    ledcWrite(CH_LEFT_LPWM, 0);
}
else if (speed < 0) {
    ledcWrite(CH_LEFT_RPWM, 0);
    ledcWrite(CH_LEFT_LPWM, abs(speed));
}
else {
    ledcWrite(CH_LEFT_RPWM, 0);
    ledcWrite(CH_LEFT_LPWM, 0);
}
 

}

void setRightMotor(int speed) {
speed = constrain(speed, -255, 255);

 
if (speed > 0) {
    ledcWrite(CH_RIGHT_RPWM, speed);
    ledcWrite(CH_RIGHT_LPWM, 0);
}
else if (speed < 0) {
    ledcWrite(CH_RIGHT_RPWM, 0);
    ledcWrite(CH_RIGHT_LPWM, abs(speed));
}
else {
    ledcWrite(CH_RIGHT_RPWM, 0);
    ledcWrite(CH_RIGHT_LPWM, 0);
}
 

}

void stopMotors() {
setLeftMotor(0);
setRightMotor(0);
}

// =========================
// DRIVE MIXER
// =========================
void drive(int throttle, int steering) {

 
int left  = throttle + steering;
int right = throttle - steering;

left  = constrain(left, -255, 255);
right = constrain(right, -255, 255);

setLeftMotor(left);
setRightMotor(right);
 

}

// =========================
// RESPONSE CURVE
// =========================
float applyCurve(float input) {

 
float normalized = input / 512.0f;
float curved = normalized *
               normalized *
               normalized;

return curved * 512.0f;
 

}

// =========================
// BUTTON HANDLER
// =========================
void checkButtons() {

 
bool armBtn = digitalRead(ARM_BTN);

if (armBtn == LOW && lastArmBtnState == HIGH) {

    robotEnabled = !robotEnabled;

    digitalWrite(ARM_LED,
                 robotEnabled ? HIGH : LOW);

    if (!robotEnabled)
        stopMotors();

    Serial.print("Robot State: ");
    Serial.println(
        robotEnabled ?
        "ENABLED" :
        "DISABLED"
    );
}

lastArmBtnState = armBtn;

bool repairBtn = digitalRead(BT_REPAIR_BTN);

if (repairBtn == LOW &&
    lastRepairBtnState == HIGH) {

    Serial.println("Bluetooth Repair Requested");

    stopMotors();

    digitalWrite(BT_LED, LOW);

    delay(100);

    BP32.forgetBluetoothKeys();

    Serial.println("Restarting ESP32");

    delay(500);

    ESP.restart();
}

lastRepairBtnState = repairBtn;
 

}

// =========================
// PROCESS GAMEPAD
// =========================
void processGamepad(ControllerPtr ctl) {

 
if (!robotEnabled) {
    stopMotors();
    return;
}

// Left stick = throttle
int ly = ctl->axisY();

// Right stick = steering
int rx = ctl->axisRX();

uint8_t dpad = ctl->dpad();

int throttle = 0;
int steering = 0;

// ---------------------
// D-PAD OVERRIDE
// ---------------------
if (dpad != 0) {

    if (dpad & DPAD_UP)
        throttle = 255;

    if (dpad & DPAD_DOWN)
        throttle = -255;

    if (dpad & DPAD_LEFT)
        steering = -255;

    if (dpad & DPAD_RIGHT)
        steering = 255;
}
else {

    if (abs(ly) < 30)
        ly = 0;

    if (abs(rx) < 30)
        rx = 0;

    float curvedLY = applyCurve(ly);
    float curvedRX = applyCurve(rx);

    throttle =
        map((int)-curvedLY,
            -512,
            512,
            -255,
            255);

    steering =
        map((int)curvedRX,
            -512,
            512,
            -255,
            255);
}

int correctedThrottle = steering;
int correctedSteering = -throttle;

drive(correctedThrottle, correctedSteering);

static uint32_t lastPrint = 0;

if (millis() - lastPrint > 200) {

    Serial.printf(
        "Throttle:%4d Steering:%4d Enabled:%d\n",
        throttle,
        steering,
        robotEnabled
    );

    lastPrint = millis();
}
 

}

// =========================
// SETUP
// =========================
void setup() {

 
Serial.begin(115200);

pinMode(ARM_LED, OUTPUT);
pinMode(BT_LED, OUTPUT);

pinMode(ARM_BTN, INPUT_PULLUP);
pinMode(BT_REPAIR_BTN, INPUT_PULLUP);

digitalWrite(ARM_LED, LOW);
digitalWrite(BT_LED, LOW);

ledcSetup(CH_LEFT_RPWM,
          PWM_FREQUENCY,
          PWM_RESO);

ledcSetup(CH_LEFT_LPWM,
          PWM_FREQUENCY,
          PWM_RESO);

ledcSetup(CH_RIGHT_RPWM,
          PWM_FREQUENCY,
          PWM_RESO);

ledcSetup(CH_RIGHT_LPWM,
          PWM_FREQUENCY,
          PWM_RESO);

ledcAttachPin(
    LEFT_RPWM,
    CH_LEFT_RPWM
);

ledcAttachPin(
    LEFT_LPWM,
    CH_LEFT_LPWM
);

ledcAttachPin(
    RIGHT_RPWM,
    CH_RIGHT_RPWM
);

ledcAttachPin(
    RIGHT_LPWM,
    CH_RIGHT_LPWM
);

stopMotors();

BP32.setup(
    &onConnectedController,
    &onDisconnectedController
);

Serial.println("Waiting for Controller...");
 

}

// =========================
// LOOP
// =========================
void loop() {

 
checkButtons();

bool dataUpdated = BP32.update();

if (dataUpdated &&
    myController &&
    myController->isConnected()) {

    processGamepad(myController);
}

delay(5);
 

}
