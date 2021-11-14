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
} Axle;


// Specify number of shields with numShields
// Specify shield address with shieldPins
const int numShields = 1;
const uint8_t shieldPins[numShields] = {0x60};

// Specify number of motors on each shield with numMotors
// Specify motor pin and button pins with motorPins and buttonPins
// motorPins[i] length and buttonPins[i] lengt should equal numMotors[i]
const uint8_t numMotors[numShields] = {1};
const uint8_t motorPins[numShields][4] = {{1}};
const uint8_t buttonPins[numShields][4] = {{0}};

// Should be `sum(numMotors)`
const int numAxles = 1;
Axle *axles[numAxles];

const unsigned long debounceDelay = 50;

const float maxRotations = 0.5;
const float minAngle = 0.0;
const float maxAngle = 360.0 * maxRotations;

const int numCharsPerMotor = 5;
const byte numChars = numAxles * numCharsPerMotor;
char receivedChars[numChars];
boolean newData = false;


void setup() {
	Serial.begin(9600);

	int axleIdx = 0;
	for (int i = 0; i < numShields; i++) {
		Adafruit_MotorShield AFMS = Adafruit_MotorShield(shieldPins[i]);
		if (!AFMS.begin()) {
			Serial.print("Could not find Motor Shield ");
			Serial.println(i);
			while (1);
		}

		for (int j = 0; j < numMotors[i]; j++) {
			pinMode(buttonPins[i][j], INPUT);
			Button button = {
				.pin = buttonPins[i][j],
				.lastDebounceTime = 0,
				.lastReading = LOW,
				.state = LOW,
			};
			Axle axle = {
				.motor = AFMS.getMotor(motorPins[i][j]),
				.button = &button,
				.lastTime = 0,
				.angle = 0,
				.motorSpeed = 0,
			};
      delay(0);  // DONT TOUCH
			axles[axleIdx++] = &axle;
		}
	}

	if (axleIdx != numAxles) {
		Serial.print("Only "); Serial.print(axleIdx);
		Serial.println(" axles setup. Ensure numMotors is set properly.");
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
        Serial.print("Test");
				receivedChars[ndx] = '\0'; // terminate the string
        Serial.print("Received chars: "); Serial.println(receivedChars);
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


float speed2angle(int motorSpeed) {
	/* Converts motor speed to angles per millisecond */
	float rpm = ((float)motorSpeed / 255.0) * 380.0; // Linear increase from 0 - 380 RPM
	float anglesPerMinute = rpm * 360.0;
	float anglesPerSecond = (float)anglesPerMinute / 60.0;
	float anglesPerMilli = (float)anglesPerSecond / 1000.0;
	return anglesPerMilli;
}


void updateAxleAngle(Axle *axle) {
	// The conversion from unsigned long (lastTime and millis()) to float
	// (axle->angle) is potentially a problem, but in practice shouldn't be since
	// the time between loops is small.
  unsigned long currentTime = millis();
  Serial.print("Change: "); Serial.println(speed2angle(axle->motorSpeed));
  Serial.print("Before: "); Serial.println(axle->angle);
	axle->angle += (float)(currentTime - axle->lastTime) * speed2angle(axle->motorSpeed);
  Serial.print("After: "); Serial.println(axle->angle);
  axle->lastTime = currentTime;
  Serial.println();
}

void updateAngles() {
  for (int i = 0; i < numAxles; i++){
    updateAxleAngle(axles[i]);
    if (
      (axles[i]->angle < minAngle && axles[i]->motorSpeed < 0)||
      (axles[i]->angle > maxAngle && axles[i]->motorSpeed > 0)
    ) {
      setMotor(axles[i], 0, RELEASE);
    }
  }
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


void setMotor(Axle *axle, int motorSpeed, int dir) {
	/* Sets motor speed and direction

		 Exists to ensure we never update the motor speed without also updating the
		 speed attribute of the axle.
	 */
	axle->motorSpeed = motorSpeed * (dir == FORWARD ? 1 : -1);
	axle->motor->run(dir);
	axle->motor->setSpeed(motorSpeed);
}


void parseNewData() {
	for (int i = 0; i < numAxles; i++) {
		char * token = strtok(i == 0 ? receivedChars : NULL, ",");
		int motorSpeed = atoi(token);
		int dir = FORWARD;
    
    updateAxleAngle(axles[i]);
		updateButton(axles[i]->button);

		if (
			//axles[i]->button->state == HIGH ||
			(axles[i]->angle < minAngle && motorSpeed < 0)||
			(axles[i]->angle > maxAngle && motorSpeed > 0)
		) {
			motorSpeed = 0;
			dir = RELEASE;
		}
		else if (motorSpeed < 0) {
			motorSpeed *= -1;
			dir = BACKWARD;
		}

		setMotor(axles[i], motorSpeed, dir);
		newData = false;
	}
}


void loop() {
	recvWithStartEndMarkers();
  
	if (newData) { 
		parseNewData();
	}
	else {
    updateAngles();
	}
  Serial.println(axles[0]->angle);
}
