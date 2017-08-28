/*
 * Copyright (c) 2017, Luc Yriarte
 * License: BSD <http://www.opensource.org/licenses/bsd-license.php>
 */

#include <Servo.h> 

/* **** **** **** **** **** ****
 * Constants
 * **** **** **** **** **** ****/

#define BPS_HOST 9600
#define COMMS_BUFFER_SIZE 256

/* 
 * ultra sonic telemeter mesure
 */
#define INECHO 2
#define TRIGGER 3
#define ECHO_TIMEOUT 100000
#define ECHO2CM(x) (x/60)
#define MAX_CM 1000

/* 
 * servo
 */
#define SRV 7

/* 
 * automaton states
 */
enum {
	START,
	ERROR,
	IN_CMD,
	IN_SERVO,
	IN_STEPPER,
	SENSOR_MESURE
};


/* **** **** **** **** **** ****
 * Global variables
 * **** **** **** **** **** ****/

/* 
 * servo
 */
Servo servo;

/* 
 * automaton status
 */
int currentState;

/* 
 * serial comms buffer
 */
char commsBuffer[COMMS_BUFFER_SIZE];

/* **** **** **** **** **** ****
 * Functions
 * **** **** **** **** **** ****/

void setup() {
	pinMode(INECHO, INPUT);
	pinMode(TRIGGER, OUTPUT);
	servo.attach(SRV);
	Serial.begin(BPS_HOST);
	digitalWrite(TRIGGER, LOW);
	currentState = START;
}

/* 
 * servo command
 */
void servoCommand(int angle) {
	servo.write(angle);
}

/* 
 * telemeter mesure
 */
int telemeterMesure() {
	unsigned long echoDuration;
	unsigned int mesureCm;
	digitalWrite(TRIGGER, HIGH);
	delayMicroseconds(10);
	digitalWrite(TRIGGER, LOW);
	echoDuration = pulseIn(INECHO, HIGH, ECHO_TIMEOUT);
	mesureCm = echoDuration ? ECHO2CM(echoDuration) : MAX_CM;
	return mesureCm;
}

/* 
 * userIO
 */
char * userInput(char * message) {
	char * input = NULL;
	int nread = 0;
	Serial.print(message);
	while (!(nread = Serial.readBytes(commsBuffer, COMMS_BUFFER_SIZE)));
	if (nread == COMMS_BUFFER_SIZE)
		return input; // buffer overflow
	commsBuffer[nread] = 0;
	input = commsBuffer;
	return input;
}

/* 
 * automaton
 */
int stateTransition(int currentState) {
	int newState = ERROR;
	char * input = NULL;
	int value = 0;
	switch (currentState) {
		case START:
			newState = IN_CMD;
			break;

		case ERROR:
			Serial.println("ERROR");
			newState = START;
			break;

		case IN_CMD:
			if (!(input = userInput("CMD: ")))
				break;
				
			if (!strcmp(input,"SERVO"))
				newState = IN_SERVO;
			else if (!strcmp(input,"STEPPER"))
				newState = IN_STEPPER;
			else if (!strcmp(input,"MESURE"))
				newState = SENSOR_MESURE;
			break;

		case IN_SERVO:
			if (!(input = userInput("ANGLE: ")))
				break;
			value = atoi(input);
			servoCommand(value);
			newState = IN_CMD;
			break;

		case IN_STEPPER:
			if (!(input = userInput("STEPS: ")))
				break;
			value = atoi(input);
			newState = IN_CMD;
			break;

		case SENSOR_MESURE:
			value = telemeterMesure();
			Serial.println(value);
			newState = IN_CMD;
			break;

		default:
			newState = ERROR;
			break;
	}
	return newState;
}

void loop() {
	currentState = stateTransition(currentState);
}
