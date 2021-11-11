#include <Adafruit_MotorShield.h>

typedef struct {
	uint8_t pin;
	unsigned long lastDebounceTime;
	int lastReading;
	int state;
} Button;

typedef struct {
	Adafruit_DCMotor *motor;
	int lastSpeed;
	int angle;
	Button *button;
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
				.lastSpeed = 0,
				.angle = 0,
				.button = &button,
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
	 *
	 * Reads until it identifies a start marker. Once it does, continues
	 * reading, now copying into a buffer. Stops when the end marker is found.
	 * Sets newData flag when data has finished reading.
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


int readButton(Button *button) {
	/* Debounces the button.
	 *
	 * Only updates the button state when the reading has been consistent for
	 * at least debounceDelay milliseconds.
	 */
	int reading = digitalRead(button->pin);
	if (reading != button->lastReading) {
		button->lastDebounceTime = millis();
	}
	if ((millis() - button->lastDebounceTime) > debounceDelay) {
		button->state = reading;
	}
	button->lastReading = reading;
	return button->state;
}


void parseNewData() {
	for (int i = 0; i < numAxles; i++){
		char * token = strtok(i == 0 ? receivedChars : NULL, ",");
		int motorSpeed = atoi(token);
		int direction = FORWARD;
		int limitSwitchReading = readButton(axles[i]->button);

		if (limitSwitchReading == HIGH) {
			motorSpeed = 0;
			direction = RELEASE;
		}
		else if (motorSpeed < 0) {
			motorSpeed *= -1;
			direction = BACKWARD;
		}

		axles[i]->motor->run(direction);
		axles[i]->motor->setSpeed(motorSpeed);
		newData = false;
	}
}


void loop() {
	recvWithStartEndMarkers();
	if (newData) { 
		parseNewData();
	}
}
