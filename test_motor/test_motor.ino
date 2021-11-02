/*
This is a test sketch for the Adafruit assembled Motor Shield for Arduino v2
It won't work with v1.x motor shields! Only for the v2's with built in PWM
control
For use with the Adafruit Motor Shield v2
---->  http://www.adafruit.com/products/1438
*/

#include <Adafruit_MotorShield.h>

const int numMotors = 1;
const int bufferLength = 3;

char buff [bufferLength + 1] = {};

// Create the motor shield object with the default I2C address
Adafruit_MotorShield AFMS = Adafruit_MotorShield();
// Or, create it with a different I2C address (say for stacking)
// Adafruit_MotorShield AFMS = Adafruit_MotorShield(0x61);

Adafruit_DCMotor *motors [numMotors];

// Select which 'port' M1, M2, M3 or M4. In this case, M1
// Adafruit_DCMotor *myMotor = AFMS.getMotor(1);
// You can also make another motor on port M2
//Adafruit_DCMotor *myOtherMotor = AFMS.getMotor(2);

void setup() {
  Serial.begin(9600);           // set up Serial library at 9600 bps
  Serial.println("Adafruit Motorshield v2 - DC Motor test!");


  for (int i = 0; i <= numMotors; i++) {
    motors[i] = AFMS.getMotor(i + 1);
  }


  if (!AFMS.begin()) {         // create with the default frequency 1.6KHz
  // if (!AFMS.begin(1000)) {  // OR with a different frequency, say 1KHz
    Serial.println("Could not find Motor Shield. Check wiring.");
    while (1);
  }
  Serial.println("Motor Shield found.");


}

void loop() {
  if (Serial.available() > 2) { 
    for (int i = 0; i < numMotors; i++){


      Serial.readBytes(buff, bufferLength);
      buff[bufferLength] = '\0';
      int motorSpeed = atoi(buff);
 
      Serial.print(motorSpeed);
      motors[i]->run(FORWARD);
      motors[i]->setSpeed(motorSpeed);

    }

    
  }
}
