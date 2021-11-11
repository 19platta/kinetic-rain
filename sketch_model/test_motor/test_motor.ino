/*
This is a test sketch for the Adafruit assembled Motor Shield for Arduino v2
It won't work with v1.x motor shields! Only for the v2's with built in PWM
control
For use with the Adafruit Motor Shield v2
---->  http://www.adafruit.com/products/1438
*/

#include <Adafruit_MotorShield.h>

const int numMotors = 1;
Adafruit_DCMotor *motors [numMotors];
Adafruit_MotorShield AFMS = Adafruit_MotorShield();

const int numCharsPerMotor = 5;
const byte numChars = numMotors * numCharsPerMotor;
char receivedChars[numChars];
boolean newData = false;

void setup() {
  Serial.begin(9600);           // set up Serial library at 9600 bps
  Serial.println("Adafruit Motorshield v2 - DC Motor test!");


  for (int i = 0; i < numMotors; i++) {
    motors[i] = AFMS.getMotor(i + 1);
  }


  if (!AFMS.begin()) {         // create with the default frequency 1.6KHz
  // if (!AFMS.begin(1000)) {  // OR with a different frequency, say 1KHz
    Serial.println("Could not find Motor Shield. Check wiring.");
    while (1);
  }
  Serial.println("Motor Shield found.");


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

void parseNewData() {
  for (int i = 0; i < numMotors; i++){
    char * token = strtok(i == 0 ? receivedChars : NULL, ",");
    int motorSpeed = atoi(token);
    Serial.println(motorSpeed);
    if (motorSpeed > 0) {
      motors[i]->run(FORWARD);
    } else {
      motors[i]->run(BACKWARD);
      motorSpeed *= -1;
    }
    motors[i]->setSpeed(motorSpeed);
    newData = false;
  }
}

void loop() {
  recvWithStartEndMarkers();
  
  if (newData) { 
    parseNewData();
  }
}
