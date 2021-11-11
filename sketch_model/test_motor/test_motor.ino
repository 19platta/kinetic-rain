/*
This is a test sketch for the Adafruit assembled Motor Shield for Arduino v2
It won't work with v1.x motor shields! Only for the v2's with built in PWM
control
For use with the Adafruit Motor Shield v2
---->  http://www.adafruit.com/products/1438
*/

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

const int numAxles = 1;
Axle *axles[numAxles];

Adafruit_MotorShield AFMS = Adafruit_MotorShield();
const uint8_t motorPins[numAxles] = {1};

const uint8_t buttonPins[numAxles] = {0};
const unsigned long debounceDelay = 50;

const int numCharsPerMotor = 5;
const byte numChars = numAxles * numCharsPerMotor;
char receivedChars[numChars];
boolean newData = false;


void setupAxles() {
  for (int i = 0; i < numAxles; i++) {
		pinMode(buttonPins[i], INPUT);
		Button button = {
			.pin = buttonPins[i],
			.lastDebounceTime = 0,
			.lastReading = LOW,
			.state = LOW,
		};
		Axle axle = {
			.motor = AFMS.getMotor(motorPins[i]),
			.lastSpeed = 0,
			.angle = 0,
			.button = &button,
		};
    axles[i] = &axle;
  }
}


void setup() {
  Serial.begin(9600);

  if (!AFMS.begin()) {
    Serial.println("Could not find Motor Shield. Check wiring.");
    while (1);
  }

	setupAxles();

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
	 * Only updates the button state when the reading has * been consistent for
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
