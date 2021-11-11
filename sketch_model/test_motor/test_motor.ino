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
	long angle;
	int speed;
} Axle;


// Specify number of shields with numShields
// Specify shield address with shieldPins
const int numShields = 1;
const uint8_t shieldPins[numShields] = {0x60};

// Specify number of motors on each shield with numMotors
// Specify motor pin and button pins with motorPins and buttonPins
// motorPins[i] length and buttonPins[i] lengt should equal numMotors[i]
const uint8_t numMotors[numShields] = {1};
const uint8_t motorPins[numShields][4] = {{1,}};
const uint8_t buttonPins[numShields][4] = {{0,}};

// Should be `sum(numMotors)`
const int numAxles = 1;
Axle *axles[numAxles];

const unsigned long debounceDelay = 50;

const int minAngle = 0;
const int maxAngle = 360 * 1000;

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
				.speed = 0,
			};
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


long speed2angle(int speed) {
	/* Converts motor speed to angles per millisecond */
	int rpm = (((long)speed) / 255) * 200; // Linear increase from 0 - 200 RPM
	int anglesPerMinute = rpm * 360;
	int anglesPerSecond = anglesPerMinute / 60;
	int anglesPerMilli = anglesPerSecond / 1000;
	return anglesPerMilli;
}


void updateAngle(Axle *axle) {
	// The conversion from unsigned long (lastTime and millis()) to signed long
	// (axle->angle) is potentially a problem, but in practice shouldn't be since
	// the time between loops is small.
	axle->angle += ((long)(millis() - axle->lastTime)) * speed2angle(axle->speed);
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


void setMotor(Axle *axle, int speed, int direction) {
	/* Sets motor speed and direction

		 Exists to ensure we never update the motor speed without also updating the
		 speed attribute of the axle.
	 */
	axle->speed = speed * (direction == FORWARD ? 1 : -1);
	axle->motor->run(direction);
	axle->motor->setSpeed(speed);
}


void parseNewData() {
	for (int i = 0; i < numAxles; i++){
		char * token = strtok(i == 0 ? receivedChars : NULL, ",");
		int motorSpeed = atoi(token);
		int direction = FORWARD;

		updateAngle(axles[i]);
		updateButton(axles[i]->button);

		if (
			axles[i]->button->state == HIGH ||
			axles[i]->angle < minAngle ||
			axles[i]->angle > maxAngle
		) {
			motorSpeed = 0;
			direction = RELEASE;
		}
		else if (motorSpeed < 0) {
			motorSpeed *= -1;
			direction = BACKWARD;
		}

		setMotor(axles[i], motorSpeed, direction);
		newData = false;
	}
}


void loop() {
	recvWithStartEndMarkers();
	if (newData) { 
		parseNewData();
	}
}
