#include <Adafruit_MotorShield.h>

typedef struct {
  uint8_t pin;
  unsigned long lastDebounceTime;
  int lastReading;
  int state;
} Button;

typedef struct {
  Adafruit_DCMotor *motor;
  Button *button;
  unsigned long lastTime;
  float angle;
  int motorSpeed;
  int motorDir;
} Axle;


// Specify number of shields with numShields
// Specify shield address with shieldPins
const int numShields = 1;
const uint8_t shieldPins[numShields] = {0x60};

// Specify number of motors on each shield with numMotors
// Specify motor pin and button pins with motorPins and buttonPins
// motorPins[i] length and buttonPins[i] lengt should equal numMotors[i]
const uint8_t numMotors[numShields] = {2};
const uint8_t motorPins[numShields][4] = {{1, 3}};
const uint8_t buttonPins[numShields][4] = {{0, 0}};

// Should be `sum(numMotors)`
const int numAxles = 2;
Axle *axles[numAxles];

const unsigned long debounceDelay = 50;

const float maxRotations = 100;
const float minAngle = 0.0;
const float maxAngle = 360.0 * maxRotations;

const int numCharsPerAxle = 5;
const byte numChars = numAxles * numCharsPerAxle + 2;
char receivedChars[numChars];
boolean newData = false;


void setup() {
  Serial.begin(9600);

  int axleIdx = 0;
  for (int i = 0; i < numShields; i++) {
    Adafruit_MotorShield AFMS = Adafruit_MotorShield(shieldPins[i]);
    if (!AFMS.begin()) {
      Serial.print("Could not find Motor Shield "); Serial.println(i);
      while (1);
    }

    for (int j = 0; j < numMotors[i]; j++) {
      pinMode(buttonPins[i][j], INPUT);
      Button *button = new Button {
        .pin = buttonPins[i][j],
        .lastDebounceTime = 0,
        .lastReading = LOW,
        .state = LOW,
      };
      Axle *axle = new Axle {
        .motor = AFMS.getMotor(motorPins[i][j]),
          .button = button,
          .lastTime = 0,
          .angle = 0,
          .motorSpeed = 0,
          .motorDir = RELEASE,
      };
      axles[axleIdx++] = axle;
      // DO NOT TOUCH
      // Running with one motor breaks without this line
      delay(0); 
    }
  }

  if (axleIdx != numAxles) {
    Serial.print(axleIdx); Serial.println(" axles setup. ");
    Serial.print(numAxles); Serial.println(" axles expected. ");
    Serial.println("Ensure numMotors  and numAxles are set properly.");
    while (1);
  }

  Serial.println("Setup Done");
}


void recvWithStartEndMarkers() {
  /* Reads data from serial, expecting start and end markers

     Reads until it identifies a start marker. Once it does, continues
     reading, now copying into a buffer. Stops when the end marker is found.
     Sets newData flag when data has finished reading.
   */
  static boolean recvInProgress = false;
  static byte ndx = 0;
  char startMarker = '<';
  char endMarker = '>';
  char rc;
  while (Serial.available() > 0 && newData == false) {
    rc = Serial.read();

    if (recvInProgress == true) {
      if (rc != endMarker) {
        receivedChars[ndx] = rc;
        ndx++;
        if (ndx >= numChars) {
          ndx = numChars - 1;
        }
      }
      else {
        receivedChars[ndx] = '\0'; // terminate the string
        recvInProgress = false;
        ndx = 0;
        newData = true;
      }
    }
    else if (rc == startMarker) {
      recvInProgress = true;
    }
  }
}


void setMotor(Axle *axle, int motorSpeed, int dir) {
  /* Sets motor speed and direction

     Exists to ensure we never update the motor speed without also updating the
     speed attribute of the axle.
   */
  axle->motorDir = dir;
  axle->motorSpeed = motorSpeed;
  axle->motor->run(dir);
  axle->motor->setSpeed(motorSpeed);
}


float speed2angle(int motorSpeed) {
  /* Converts motor speed to angles per millisecond */
  float rpm = ((float)motorSpeed / 255.0) * 380.0; // Linear increase from 0 - 380 RPM
  float anglesPerMinute = rpm * 360.0;
  float anglesPerSecond = (float)anglesPerMinute / 60.0;
  float anglesPerMilli = (float)anglesPerSecond / 1000.0;
  return anglesPerMilli;
}


void updateAngle(Axle *axle) {
  // The conversion from unsigned long (lastTime and millis()) to float
  // (axle->angle) is potentially a problem, but in practice shouldn't be since
  // the time between loops is small.
  unsigned long currentTime = millis();
  int motorSpeed = axle->motorSpeed * (axle->motorDir == FORWARD ? 1 : -1);
  axle->angle += (float)(currentTime - axle->lastTime) * speed2angle(motorSpeed);
  axle->lastTime = currentTime;
}


void updateButton(Button *button) {
  /* Debounces the button.

     Only updates the button state when the reading has been consistent for
     at least debounceDelay milliseconds.
   */
  int reading = digitalRead(button->pin);
  if (reading != button->lastReading) {
    button->lastDebounceTime = millis();
  }
  if ((millis() - button->lastDebounceTime) > debounceDelay) {
    button->state = reading;
  }
  button->lastReading = reading;
}


void updateAxles() {
  for (int i = 0; i < numAxles; i++) {
    int motorSpeed = axles[i]->motorSpeed;
    int dir = axles[i]->motorDir;

    updateAngle(axles[i]);
    updateButton(axles[i]->button);

    // Parse new data
    if (newData) {
      char *token = strtok(i == 0 ? receivedChars : NULL, ",>");
      motorSpeed = atoi(token);
      dir = FORWARD;
      if (motorSpeed < 0) {
        motorSpeed *= -1;
        dir = BACKWARD;
      }
    }

    // Stop conditions
    if (
        //axles[i]->button->state == HIGH ||
        (axles[i]->angle <= minAngle && dir == BACKWARD)||
        (axles[i]->angle >= maxAngle && dir == FORWARD)
       ) {
      motorSpeed = 0;
      dir = RELEASE;
    }

    // Update motor (only if something differs)
    if (motorSpeed != axles[i]->motorSpeed || dir != axles[i]->motorDir) {
      setMotor(axles[i], motorSpeed, dir);
    }
  }
  newData = false;
}


void loop() {
  recvWithStartEndMarkers();
  updateAxles();
}
