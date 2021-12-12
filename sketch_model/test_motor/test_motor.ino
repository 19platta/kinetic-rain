 #include <Adafruit_MotorShield.h>

typedef struct {
  uint8_t pin;
  unsigned long lastDebounceTime;
  int lastReading;
  int state;
  bool changed;
} Button;

typedef struct {
  Adafruit_DCMotor *motor;
  Button *button;
  unsigned long lastTime;
  float angle;
  int motorSpeed;
  float angleSpeed;
  int motorDir;
} Axle;

typedef struct {
  int motorSpeed;
  float angleSpeed;
} DataTuple;


// Specify number of shields with numShields
// Specify shield address with shieldPins
const int numShields = 1;
const uint8_t shieldPins[numShields] = {0x60};
Adafruit_MotorShield motorShields[numShields];

// Specify number of motors on each shield with numMotors
// Specify motor pin and button pins with motorPins and buttonPins
// motorPins[i] length and buttonPins[i] lengt should equal numMotors[i]
const uint8_t numMotors[numShields] = {1};
const uint8_t motorPins[numShields][4] = {{4}};
const uint8_t buttonPins[numShields][4] = {{2}};

// Should be `sum(numMotors)`
const int numAxles = 1;
Axle *axles[numAxles];

const unsigned long debounceDelay = 5;

const float maxRotations = 2.0;
const float minAngle = 0.0;
const float maxAngle = 360.0 * maxRotations;
const float encoderAngle = 90.0;

const int numCharsPerAxle = 18;
const byte numChars = numAxles * numCharsPerAxle + 2;
char receivedChars[numChars];
boolean newData = false;


void setup() {
  Serial.begin(9600);

  int axleIdx = 0;
  for (int i = 0; i < numShields; i++) {
    motorShields[i] = Adafruit_MotorShield(shieldPins[i]);
    if (!motorShields[i].begin()) {
      Serial.print("Could not find Motor Shield "); Serial.println(i);
      while (1);
    }

    for (int j = 0; j < numMotors[i]; j++) {
      pinMode(buttonPins[i][j], INPUT_PULLUP);
      Button *button = new Button {
        .pin = buttonPins[i][j],
        .lastDebounceTime = 0,
        .lastReading = LOW,
        .state = LOW,
        .changed = false,
      };
      Axle *axle = new Axle {
        .motor = motorShields[i].getMotor(motorPins[i][j]),
        .button = button,
        .lastTime = 0,
        .angle = 0,
        .motorSpeed = 0,
        .angleSpeed = 0.0,
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
  char startMarker = '[';
  char endMarker = ']';
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


void setMotor(Axle *axle, int motorSpeed, float angleSpeed, int dir) {
  /* Sets motor speed and direction

     Exists to ensure we never update the motor speed without also updating the
     speed attribute of the axle.
   */
  axle->motorDir = dir;
  axle->motorSpeed = motorSpeed;
  axle->angleSpeed = angleSpeed;
  axle->motor->run(dir);
  axle->motor->setSpeed(motorSpeed);
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
  if ((millis() - button->lastDebounceTime) > debounceDelay && button->state != reading) {
    button->state = reading;
    button->changed = true;
  }
  else {
    button->changed = false;
  }
  button->lastReading = reading;
}


void updateAngle(Axle *axle) {
  if (axle->button->state == HIGH && axle->button->changed) {
    int angleDir = axle->motorDir == BACKWARD || axle->motorSpeed == 0 ? -1 : 1;
    axle->angle += encoderAngle * angleDir;
    Serial.println(axle->angle);
    Serial.println(axle->motorSpeed);
  }
}


DataTuple *parseTuple(char *tupleString) {
  /* Parses a tuple passed over serial

     Tuple is expectd to be of the form:
     (3 char int, 10 char float)
     where the int is the motor speed and the float is the angle speed
   */
  char *endChar;
  char *token;

  // Increment past leading characters
  while (
      *tupleString == '[' ||
      *tupleString == '(' || 
      *tupleString == ' ' ||
      *tupleString == ','
      ) {
    tupleString++;   
  }

  // Parse motor speed (int)
  token = strtok_r(tupleString, ",", &endChar);
  int motorSpeed = atoi(token);

  // Parse angular speed (float)
  token = strtok_r(NULL, ",", &endChar);
  float angleSpeed = atof(token);

  return new DataTuple {
    .motorSpeed = motorSpeed,
    .angleSpeed = angleSpeed
  };
}


void updateAxles() {
  char *endTuple;
  for (int i = 0; i < numAxles; i++) {
    int motorSpeed = axles[i]->motorSpeed;
    float angleSpeed = axles[i]->angleSpeed;
    int dir = axles[i]->motorDir;

    updateButton(axles[i]->button);
    updateAngle(axles[i]);

    // Parse new data
    if (newData) {
      // Parse tuple
      char *tupleString = strtok_r(i == 0 ? receivedChars : NULL, ")", &endTuple);
      DataTuple *tuple = parseTuple(tupleString);
      motorSpeed = tuple->motorSpeed;
      angleSpeed = tuple->angleSpeed;
      delete tuple;

      // Figure out direction
      dir = FORWARD;
      if (motorSpeed < 0) {
        motorSpeed *= -1;
        dir = BACKWARD;
      }
    }

    // Stop conditions
    if (
        (axles[i]->angle <= minAngle && dir == BACKWARD)||
        (axles[i]->angle >= maxAngle && dir == FORWARD)
       ) {
      motorSpeed = 0;
      angleSpeed = 0.0;
      // dir = RELEASE;
    }

    // Update motor (only if something differs)
    if (
        motorSpeed != axles[i]->motorSpeed ||
        angleSpeed != axles[i]->angleSpeed ||
        dir != axles[i]->motorDir
       ) {
      setMotor(axles[i], motorSpeed, angleSpeed, dir);
    }
  }
  newData = false;
}


void loop() {
  recvWithStartEndMarkers();
  updateAxles();
}
