
/*
* Code pour construction d'un recepteur "maison", recois un signal et ouvre ou ferme un port exterieur (relie par exemple a un relais)
* Frequence : 433.92 mhz
* Protocole : home easy
* Licence : CC -by -sa
* Materiel associe : Atmega 328 (+resonateur associe) + recepteur RF AM 433.92 mhz + relais + led d'etat
* Auteur : Valentin CARRUESCO  aka idleman (idleman@idleman.fr - http://blog.idleman.fr)
*
* Base sur le travail de :
* Barnaby Gray 12/2008
* Peter Mead   09/2009
*/

#include <EEPROM.h>

#include "EEPROMAnything.h"



const int buttonPin = 4;     // the number of the pushbutton pin
int buttonState = 0;         // variable for reading the pushbutton status

const int recepteurPin = 11;
const int commandUp = 10;
const int commandDown = 9;

int ledPin = 13;
boolean buttonReceived, signalOK, upStarted, downStarted;
const boolean buttonUp = true; 
unsigned long initStartTimeMillis, actionStartTimeMillis = 0;        // will store last time LED was updated

const long initDelay = 0; // 10000;           // init Delay (milliseconds) accept to register new signal (0 = disable)
const long moveDelay = 35000; // 10 second to open or close 
const int startCommand = 700; // delay in second to start 
const int stopCommand = 1300; // delay in second to stop

int signalMax = 6;
int registerIndex, totalRegistered;
int memoryOffset = 8;

typedef struct
{
	long sender;
	int receptor;
} config_t;

config_t signal[6];

struct signal_t
{                                         
	long sender;
	int receptor;
	boolean isSignal;
	boolean state;
} receivedSignal;


void setup()
{
	
	// resetMemory();
	
	pinMode(recepteurPin, INPUT);
	pinMode(commandUp, OUTPUT);
	pinMode(commandDown, OUTPUT);
	
	
	pinMode(ledPin, OUTPUT);
	Serial.begin(9600);
	// digitalWrite(ledPin, HIGH);
	initStartTimeMillis = millis();
	
	
	EEPROM_readAnything(0, registerIndex);
	EEPROM_readAnything(4, totalRegistered);
	
	if ((registerIndex > signalMax ) || (registerIndex > signalMax )) {
		resetMemory();
	}
	
	for ( int i =0 ; i < signalMax; i++ ) {
		EEPROM_readAnything((i*10)+memoryOffset, signal[i]);
	}
	signalOK = false;
	
	Serial.print("registerIndex " );
	Serial.println( registerIndex);
	
	Serial.print("totalRegistered ");
	Serial.println(totalRegistered);
	
	Serial.println(" start ");
	upStarted = false;
	downStarted = false;
	blinkLed(10, 50);

}


void loop()
{

	unsigned long currentTimeMillis = 0;
	//Ecoute des signaux
	listenSignal();

	//Si un signal au protocol home easy est reÁu...
	if (receivedSignal.isSignal) {
		//Recuperation du signal de reference en memoire interne
		
		currentTimeMillis = millis();	
		boolean init = (currentTimeMillis - initStartTimeMillis) < initDelay;
		if (init) {
			//if (buttonState == HIGH) {
			//Si aucun signal de reference, le premier signal reÁu servira de reference en memoire interne
			Serial.println("Init en cours");
			
			if ( registerIndex == signalMax ) {
				// on revient au 1er
				Serial.println("Memory full, reset");
				registerIndex = 0;
			} 
			
			EEPROM_writeAnything((registerIndex*10)+memoryOffset, receivedSignal);
			Serial.print("enregistrement du signal # ");
			Serial.println(registerIndex);
							blinkLed(5, 25);

			EEPROM_readAnything((registerIndex*10)+memoryOffset, signal[registerIndex]);
			// set register index for the next time
			registerIndex++;
			EEPROM_writeAnything(0, registerIndex);
			if (totalRegistered < signalMax) {
				totalRegistered++;
				EEPROM_writeAnything(4, totalRegistered);
				Serial.print("totalRegistered == ");
				Serial.println(totalRegistered);

			}
			
		} else {
			// Serial.println("Init fini");
		}
		
		signalOK = false;
		
		//Comparaison signal de reference, signal recu
		for ( int i =0; !signalOK && (i < totalRegistered); i++) {
			if (receivedSignal.sender == signal[i].sender && receivedSignal.receptor == signal[i].receptor) {
				//Serial.print("Signal correspondant # ");
				//Serial.println(i);
				
				buttonReceived = receivedSignal.state;
				//La led clignotte 10 fois rapidemment pour signaler que le signal est le bon
				//blinkLed(10, 20);
				signalOK = true;	
			} 
		}
		
		//noInterrupts();
		
		if (signalOK && !init) {
			// check if command in progress
			
			currentTimeMillis = millis();	


			if (upStarted && ((currentTimeMillis - actionStartTimeMillis) < moveDelay)) {
				Serial.println("Up is in progres send Stop");
				pushButton(commandUp, stopCommand);

			} else if (downStarted && ((currentTimeMillis - actionStartTimeMillis) < moveDelay)) {
				Serial.println("Down is in progres send Stop");				
				pushButton(commandDown, stopCommand);
			}				
			
			if (buttonReceived == buttonUp)
			{
				if (!upStarted) {
				    Serial.println("Up pressed send Start");
				    actionStartTimeMillis	= millis();			
					pushButton(commandUp, startCommand);
				}
				upStarted = !upStarted;
				downStarted = false;
			}
			else
			{
				if (!downStarted) {
					Serial.println("Down pressed send Start");
				    actionStartTimeMillis	= millis();								
					pushButton(commandDown, startCommand);
				}
				downStarted = !downStarted;
				upStarted = false;
			}
		} else {
			if (!init) {
				Serial.println("Signal inconnu");
				//La led clignotte 1 fois lentement  pour signaler que le signal est mauvais mais que le protocol est le bon
				blinkLed(2, 100);
			}
		}		
		
	}
	
}


void resetMemory() {
	// write a -1 to all 1024 bytes of the EEPROM
	// reset EEPROM il faut reprogrammer  chaque dmmarrage ,  revoir !!!
	for (int i = 0; i < 1024; i++)
		EEPROM.write(i, 0);
	registerIndex = 0;
	totalRegistered = 0;
	memoryOffset = 4;
	Serial.println("resetMemory");
	
}

void blinkLed(int repeat, int time) {
	for (int i = 0; i < repeat; i++) {
		digitalWrite(ledPin, LOW);
		delay(time);
		digitalWrite(ledPin, HIGH);
		delay(time);
	}
}



void pushButton(int button, int pushDelay) {
	digitalWrite(ledPin, HIGH);
	digitalWrite(button, HIGH);
	delay(pushDelay);
	digitalWrite(button, LOW);
	digitalWrite(ledPin, LOW);

	if (button == commandUp) {
		Serial.print("pushButton  UP :");
	} 
	else if (button == commandDown) {
		Serial.print("pushButton  Down:");
	} 
	else {
		Serial.print("pushButton  ???:");
	}
	Serial.println(pushDelay, DEC);
}


void setUpCommand(boolean state) {
	upStarted = state;
	digitalWrite(commandUp, state);
	Serial.print("Etat command UP :");
	Serial.println(upStarted);
}


void listenSignal() {
	//Serial.println("start listenSignal");

	receivedSignal.sender = 0;
	receivedSignal.receptor = 0;
	receivedSignal.isSignal = false;
	
	int i = 0;
	unsigned long t = 0;
	
	byte prevBit = 0;
	byte bit = 0;
	
	unsigned long sender = 0;
	bool group = false;
	bool on = false;
	unsigned int recipient = 0;
	
	// latch 1
	while ((t < 9200 || t > 11350))
	{ 
		t = pulseIn(recepteurPin, LOW, 1000000);
	}
	//Serial.println("latch 1");
	
	// latch 2
	while (t < 2550 || t > 2800)
	{ 
		t = pulseIn(recepteurPin, LOW, 1000000);
	}
	
	//Serial.println("latch 2");
	
	// data
	while (i < 64)
	{
		t = pulseIn(recepteurPin, LOW, 1000000);
		
		if (t > 200 && t < 365)
		{ bit = 0;
			
		}
		else if (t > 1000 && t < 1400)
		{ bit = 1;
			
		}
		else
		{ i = 0;
			//Serial.print("bit mort=");
			//Serial.println(t);
			break;
		}
		
		if (i % 2 == 1)
		{
			if ((prevBit ^ bit) == 0)
			{ // must be either 01 or 10, cannot be 00 or 11
				i = 0;
     			//Serial.print("prev Bit error=");
	     		//Serial.println(t);
				break;
			}
			
			if (i < 53)
			{ // first 26 data bits
				sender <<= 1;
				sender |= prevBit;
			}
			else if (i == 53)
			{ // 26th data bit
				group = prevBit;
			}
			else if (i == 55)
			{ // 27th data bit
				on = prevBit;
			}
			else
			{ // last 4 data bits
				recipient <<= 1;
				recipient |= prevBit;
			}
		}
		
		prevBit = bit;
		++i;
	}
	
	
	// interpret message
	if (i > 0)
	{
		receivedSignal.sender = sender;
		receivedSignal.receptor = recipient;
		receivedSignal.isSignal = true;
		if (on)
		{
			receivedSignal.state = true;
		} else {
			receivedSignal.state = false;
		}
	}
}
