#include <CapacitiveSensor.h>
#include <Servo.h>

//Search the number to locate this process in code: ex: 1a.
//1a. - 1b. Close Lid 
//2. If lid closed for three seconds, lock latch.
//3. If locked, pick a game mode, but also check our lid circuit connection
//3a. If circuit is broken while locked, sound alarm
//3b. If circuit is not broken, user has 10 seconds to decide knock or game
//4. Give user 5 seconds to get ready for mode.
//5aa. Game Mode: generate random keypress sequence and play it back
//5ab. Let user create a matching keypress sequence from touch.
//5ba. Knock Mode: record users knock and rhythm;
//5bb. Listen for knock and record attempt rhytm, match against recorded.
//6. Unlock, reset flag variables for locking logic
//7. Repeat.

//Capacitive Sensor variables
CapacitiveSensor   cs1 = CapacitiveSensor(2, 3);
CapacitiveSensor   cs2 = CapacitiveSensor(2, 5);
CapacitiveSensor   cs3 = CapacitiveSensor(2, 4);
long cs1_lvl;
long cs2_lvl;
long cs3_lvl;
//Change this depending on how sensitivite the buttons are to responding.
long minTouchThreshold = 400;
long overallTouchThreshold = 1000;
long crossKeyThreshold = 300;
//KEEP LED PINS CONSECUTIVE FOR RAND FUNCTION
const byte redLED = 13;
const byte yellowLED = 12;
const byte greenLED = 11;
const byte servoPin = 9;
const byte switchPin = 10;
int switchVal = 0;
const byte speaker = 8;
const byte mainMidPiezo = A1;
const byte leftPiezo = A2;
const byte rightPiezo = A0;

//Servo.write(angle); 90 for lock, 0 for unlock.
Servo servo;

//LED FUNCTION/METHODS
void blinkLEDs(const int& timeOn, const int& timeOff, const int & numberOfTimes);
//turn on LED, no timer
void redLEDOn();
//turn on LED, no timer
void yellowLEDOn();
//turn on LED, no timer
void greenLEDOn();
void redLEDt(const unsigned long& duration);
void yellowLEDt(const unsigned long& duration);
void greenLEDt(const unsigned long& duration);
void updateAllLEDs();
//END LED FUNCTION/METHOD SECTION//

//LED VARIABLES
//turns on redLED for certain amount of time;
byte redState = LOW;
boolean redOnly = false;
unsigned long redDuration = 0;
unsigned long redLEDStart = 0;

//turns on yellowLED for certain amount of time;
byte yellowState = LOW;
boolean yellowOnly = false;
unsigned long yellowDuration = 0;
unsigned long yellowLEDStart = 0;

//turns on greenLED for certain amount of time;
byte greenState = LOW;
boolean greenOnly = false;
unsigned long greenDuration = 0;
unsigned long greenLEDStart = 0;

//blinks LEDsd for specific time on and specific time off (milliseconds) for x amount of times
//with another timing process comes some time keeping variables as we need to know how long and when
//to turn on or off.
void blinkLEDs(const int& timeOn, const int& timeOff, const int& times);
boolean blinkLightsStatus = false;
unsigned long blinkElapsed = 0;
unsigned long oneBlinkInterval = 0;
unsigned long blinkStartTime = 0;
unsigned long blinkTOn = 0;
unsigned long blinkTOff = 0;
byte blinkTimes = 0;
//END LED VARIABLES


//SOUND VARIABLES
//currentPitch and previousPitch is necessary to keep track of different tones//
// to be played as we call different sound functions that demand different pitches
int currentPitch = 0;
int previousPitch = 0;

//updateSpeaker() will check this boolean when called to play alarm tone:
//this is only assigned to true in disableAlarm();
boolean playAlarm = false;

//when called, this assigns a pitch and sets a timer for the update speaker to play
//queuing something to play before it is played in updateSpeaker
// while running on a real time application frees up the arduino
// to process other logic for events that might change the pitch or duration of tone.
void playTone(const int& pitch, const unsigned long&duration);
boolean timedTonePlay = false;
unsigned long toneDuration = 0;
unsigned long toneElapsed = 0;
unsigned long startToneTime = 0;

//This method toggles a boolean variable which then queues up the chirp melody to play
// when updateSpeaker is executed.
void chirpLow();
boolean chirpLowStatus = false;
unsigned long chirpLowDuration = 1000; //1000/3 ->triplet
unsigned long chirpLowElapsed = 0;
unsigned long startChirpLow = 0;

//This method toggles a boolean variable which then queues up the chirp melody to play
// when updateSpeaker is executed.
void chirpHigh();
boolean chirpHighStatus = false;
unsigned long chirpHighDuration = 1000; //triplet
unsigned long chirpHighElapsed = 0;
unsigned long startChirpHigh = 0;

// Knock sensing variables
int knockVal = 0;
//Set the knocking thresholds
const int quietKnock = 100;
//Counts valid knocks
int numberOfKnocks = 0;

//PASSWORD VARIABLES

//USED FOR GAME
int gamePWLength = 0;
const byte maxPWLength = 10;
//Time in between keys (time user should release and not press)
unsigned long keyPressDelay[maxPWLength];
//Time length that user should hold key down
unsigned long keyPressTime[maxPWLength];

//Time in between knocks
unsigned long pwTimeBetweenKnock[maxPWLength];
unsigned long userTimeBetween[maxPWLength];
const int knockPWAccuracy = 75; // percent accuracy required to match
unsigned long knockRecordTimeOut = 3000; //3 seconds from last knock till knockPW is analyzed
unsigned long setKnockPWTimeOut = 8000; //8 seconds to initiate that first knock for PW
//This array holds the sequence of buttons to press:
//Random numbers 13-15 will be assigned at each index to a random length
//13 is red which is left button (capacitor pin 3)
//14 is yellow which is middle button (cap pin 4)
//15 is green which is right button (cap pin 5)
// see switch case in playbackPW for more info
int buttonPressSequence[maxPWLength];


//Holds time in between knocks
unsigned long timeBetweenKnock[maxPWLength];


//"locked" is a flag that determines if we should sound the alarm if the lid opens
// If someone breaks the circuit with the lid while locked is true,
// then sound the alarm.
// If it is locked and and no alarm, then proceed on game selection mode
boolean locked = false;
//#2. Turns the latch and sets alarm booleans 
void lock();

//This flag and those timer variables are necessary in preventing the sensors from reading in values
// after completing Knock or Game as the servo's movement can trigger a game right after
boolean mutePiezoSensors = false;
boolean muteCapSensors = false;
boolean muteSwitch = false;
unsigned long sleepSwitchStart = 0;
unsigned long sleepPiezo1Start = 0;
unsigned long sleepPiezoDuration = 0;
unsigned long sleepCapStart = 0;
unsigned long sleepCapDuration = 0;
unsigned long sleepSwitchDuration = 0;


void sleepPiezoSensors(unsigned long& sleepStrt, boolean& muteSnsr, const unsigned long& sleepDur);
int listenKnockOrIgnore(const byte & mainPiezoPin, const byte& leftPiezoPin, const byte&rightPiezoPin, boolean & muteStatus, const unsigned long & sleepStart, const unsigned long & pNumsleepDur);
void senseCapsOrIgnore(long& cs1_lvl, long& cs2_lvl, long& cs3_lvl, boolean & muteStatus, const unsigned long& sleepStart, const unsigned long& sleepDur);

//"beginClosedLidTimer" we want to know if the user
// is serious about locking the box before engaging the lock.
// we'll use beginClosedLidTimer to help start the time count.
boolean beginClosedLidTimer = false;

//"lidOpenedTime" tracks how much time the lid has been opened
// on for counting time open with the box is lock()'d. Timing opening
// during locked state helps prevent unecessary alarms that can happen
// from momentarily breaking the circuit on the lid sensors (shakes, vibrations).
// If the circuit is broken for more than two seconds, sound the alarm.
unsigned long lidOpenedTime = 0;

//"startedModeSelection" starts the mode selection timer.
// we are giving the user 10 seconds to pick Knock Mode or Game Mode.
boolean startedModeSelection = false;
unsigned long startDecisionTime = 0;
unsigned long startModeCountDown = 0;

// "isModeSet" is set to true if Knock Mode or Game Mode is selected
boolean isModeSet = false;

//logic variables for KnockLock Mode
boolean knockMode = false;


//logic variables for Game Mode
boolean gameMode = false;

//counts down 5 seconds to prepare user for game or knock mode then returns true.
//if lid is opened while locked then return false to sound alarm.
boolean readyToStartMode();
//if user creates and matches knocking password (or resets pw) then return true.
//if lid is opened while locked then return false to sound alarm.
boolean playKnockMode(boolean& knockMode);
//if user wins or resets pw then return true.
//if lid is opened while locked then return false to sound alarm.
boolean playGameMode();
//randomly determine the number of touches for the password
//fill capacitorHintLED[i to gamePWLength] with a random sequence of button number
//fill  keyPressTime[i to gamePWLength]  with random millisecond length
// maybe create a delay between keypresses by filling in keyPressDelay[].
boolean generateTouchPW();
//playback password so user can attempt it.
void playBackGamePW();
//#5bb. matchGamePassword is not finished but it will read in user input and compare it against arrays.
boolean matchGamePassword();

// "currentTime" tracks the system's current time.
// This value will be compared against other times previously recorded
// to determine how much time has elapsed for certain processes such as
// updating sound and lights.
unsigned long currentTime = 0;

//DEBUG INFORMATION Stuff
//print sensor some other flags
void printSetupDebug();
//print all boolean values related to locking mechanism
void printBooleanDebug();

//code in loop(){ if(once){}} executes one time only
//useful for testing functions
boolean once = false;

void setup() {
	// put your setup code here, to run once:
	
	randomSeed(analogRead(0));
	cs1.set_CS_AutocaL_Millis(0xFFFFFFFF);
	servo.attach(servoPin); //set servo pin
	servo.write(0);
	pinMode(redLED, OUTPUT);
	pinMode(yellowLED, OUTPUT);
	pinMode(greenLED, OUTPUT);
	pinMode(switchPin, INPUT);
	for (int i = 0; i < 10; i++) {
		keyPressDelay[i] = 0;
		buttonPressSequence[i] = 0;

	}
	Serial.begin(9600);

		//\\//Serial.print("The Box is unlocked! Setup() complete!");
		//\\//Serial.print("\n");
	

}

void loop() {
	// put your main code here, to run repeatedly:

	switchVal = readOrIgnoreSwitch(switchPin, muteSwitch, sleepSwitchStart, sleepSwitchDuration);
	//switchVal = digitalRead(switchPin);
	//printSetupDebug();
	currentTime = millis();
	
	
	//read and assign our cs1,2,3_lvl variables depending the sleep status
	senseCapsOrIgnore(cs1_lvl,cs2_lvl,cs3_lvl,muteCapSensors, sleepCapStart, sleepCapDuration);
	//read and assign our knockVal depending on the sleep status
	knockVal = listenKnockOrIgnore(mainMidPiezo,leftPiezo, rightPiezo, mutePiezoSensors, sleepPiezo1Start, sleepPiezoDuration);

	//#1a.
	if (switchVal == HIGH) {
		if (locked)
		{	
			//#3.
			//at this point, either the lid was locked and remain closed
			// or the lid was locked, alarm and its timer went off.
			// either way: we must reset the lid opening timer.
			beginClosedLidTimer = false;
			if (playAlarm) { disableAlarm();}
			
			if (isModeSet)
			{	
				//#4.
				if (readyToStartMode())
				{
					if (gameMode)
					{	//#5aa.
						if (playGameMode())//#5ab.
						{	
							//#6.
							chirpHigh();
							unlock();
							
							//\\//Serial.println("GameComplete");
							 
							printBooleanDebug();
						}
						else {
							soundAlarm();
						}
					}
					if (knockMode) {
						//#5ba.
						if (playKnockMode(knockMode))//#5bb.
						{
							//#6.
							unlock();
							chirpHigh();
							
							//\\//Serial.println("KnockComplete");
							
						}
						else{
							soundAlarm();
						}
					}
				}
				else
				{
					soundAlarm();
				}
			}
			else
			{	
				//#3b.
				if (!startedModeSelection) {
					startDecisionTime = millis();
					startedModeSelection = true;
					chirpLow();
					playTone(349, 100);
					blinkLEDs(500, 500, 10);
					
					//\\//Serial.println("Mode Selection Time Frame Started.");
					

				}

				if (startedModeSelection) {
					
					if (millis() - startDecisionTime >= 10000) {
						startedModeSelection = false;
						playTone(249, 500);
						locked = false;
						servo.write(0);
						
						//\\//Serial.println("Mode Selection Time Frame Ended.");
						
					}
				}

				if (cs1_lvl > minTouchThreshold || cs2_lvl > minTouchThreshold || cs3_lvl > minTouchThreshold)
				{
					gameMode = true;
					isModeSet = true;
					blinkLEDs(500, 500, 1);

					//\\//Serial.print("Game Mode Selected, cs_lvl's: ");
					//\\//Serial.print(cs1_lvl);
					//\\//Serial.print("\t");
					//\\//Serial.print(cs2_lvl);
					//\\//Serial.print("\t");
					//\\//Serial.println(cs3_lvl);

				}

				if (knockVal > quietKnock && !gameMode) //knock mode
				{	

					//\\//Serial.print("Knock Mode Selected, knockVal: ");
					//\\//Serial.println(knockVal);

					knockMode = true;
					isModeSet = true;
					blinkLEDs(500, 500, 1);
					
				}

			}
		}
		else
		{	
			//#1b.
			//start timer on confirming locking
			if (!beginClosedLidTimer) {
				beginClosedLidTimer = true;
				lidOpenedTime = millis();

				//\\//Serial.println("Waiting for lid to be closed for 3 sec...");

			}
			if (beginClosedLidTimer)
			{
				if (millis() - lidOpenedTime >= 3000)
				{	
					//#2.
					lock();

					//\\//Serial.println("Lid is locked! lock = true;");

					//lock boolean changes here
				}
			}
		}
	}
	else //circuit not closed. (switchVal == LOW).
	{
		if (locked)
		{	
			//#3a. Begin timer after circuit breaks then
			// sound off Alarm if time exceeds 2 seconds.
			if (beginClosedLidTimer) {
				beginClosedLidTimer = false;
				lidOpenedTime = currentTime;

				//\\//Serial.println("locked state, but lid is open");

			}

			if (currentTime - lidOpenedTime >= 2000)
			{
				beginClosedLidTimer = true;

				//\\//Serial.println("ALARM!");

				soundAlarm();
			}

		}
		else 
		{	
			disableAlarm();
			beginClosedLidTimer = false;
			////\\//Serial.println("IDLE");
		}

	}

	if (!once) {

		//\\//Serial.println("Once");
 // !DEBUG
		//	//blinkLEDs(250, 1750, 1);
		//	playTone(speaker, 800, 3000);
		//chirpLow();
		//chirpHigh();
		//alarmStatus = true;
		//blinkLEDs(1000,500,3);
		redLEDt(5000);
		yellowLEDt(3000);
		greenLEDt(2000);
		
		once = true;
	}

	//tone(A0, 440, 4000);
	//tone(speaker, 440, 1000);
	//USE ME TO CALIBRATE PIEZOs and Capacitors with lid open
	//if (knockVal > quietKnock) {
	//	//\\//Serial.println(knockVal);
	//}
	
	////\\//Serial.println(analogRead(leftPiezo));
	////\\//Serial.println(analogRead(mainMidPiezo));
	////\\//Serial.println(analogRead(rightPiezo));
	////\\//Serial.println(knockVal);

	////\\//Serial.print(cs1_lvl);
	////\\//Serial.print("\t");
	////\\//Serial.print(cs2_lvl);
	////\\//Serial.print("\t");
	////\\//Serial.print(cs3_lvl);
	////\\//Serial.print("\t");
	////\\//Serial.println(cs1_lvl+ cs2_lvl+ cs3_lvl);

	

	/*
	if (cs1_lvl + cs2_lvl + cs3_lvl > overallTouchThreshold)
	{
		if (cs1_lvl > crossKeyThreshold) {
			redLEDt(1000);
			playTone(208, 1000);
		}
		if (cs2_lvl > crossKeyThreshold) {
			yellowLEDt(1000);
			playTone(277, 1000);
		}
		if (cs3_lvl > crossKeyThreshold) {
			greenLEDt(1000);
			playTone(330, 1000);
		}
	}
	*/

	updateAllLEDs();
	updateSpeaker();
	//printBooleanDebug();

	

}

boolean readyToStartMode() {

	
	boolean runMode = true;
	unsigned long elapsedTime = 0;
	unsigned long elapsedFlash = 0;
	boolean flashOn = false;
	currentTime = millis();
	unsigned long startFlash = currentTime;
	unsigned long startCountdown = currentTime;

	chirpHigh();

	//\\//Serial.println("Mode starts in 5 secs...");

	while (runMode) {

		currentTime = millis();
		switchVal = digitalRead(switchPin);
		elapsedFlash = currentTime - startFlash;
		elapsedTime = currentTime - startCountdown;
		


		if (elapsedFlash < 1000 && elapsedTime > 1000) {
			if (!flashOn)
			{
				blinkLEDs(500, 500, 1);
				flashOn = true;
			}

			if (elapsedFlash < 200) {

				tone(speaker, 849);
				////\\//Serial.println("WarpTone");
			}
			else {
				noTone(speaker);
				flashOn = false;
			}
		}
		else {
			startFlash = millis();
		}
		if (switchVal == LOW)
		{
			soundAlarm();
			//\\//Serial.println("AlarmFromReady");
			//\\//Serial.print("\t");
			runMode = true;
			return false;
		}
		if (elapsedTime >= 4000)
		{	
			tone(speaker, 1028);
			delay(500);
			noTone(speaker);
			delay(500);
			runMode = false;
			return true;
		}
		updateAllLEDs();
		updateSpeaker();
	}
	updateAllLEDs();
	updateSpeaker();
	return true;
}

//True means the game cycled through
//This function will run until the user unlocks, resets via touchkeys, or the lid is broken open.
//False means it was interrupted by a breakin
boolean playKnockMode(boolean &knockMode){

	//\\//Serial.println("playKnockMode()\n");

	boolean knockPWSet = false;
	boolean unlockBox = false;
	unsigned long setKnockPWStart = millis();
	printSetupDebug();
	digitalWrite(redLED, HIGH);
	while (knockMode)
	{	

		//\\//Serial.println("--------");

		//listen for a knock if we are not in a muteSensor period
		knockVal = listenKnockOrIgnore(mainMidPiezo, leftPiezo, rightPiezo, mutePiezoSensors, sleepPiezo1Start, sleepPiezoDuration);
		senseCapsOrIgnore(cs1_lvl, cs2_lvl, cs3_lvl, mutePiezoSensors, sleepCapStart, sleepCapDuration);
		switchVal = readOrIgnoreSwitch(switchPin, muteSwitch, sleepSwitchStart, sleepSwitchDuration);
		//if that knock is legit, begin recording the sequence of knocks.
		// note this relies on processor running fast.
		//printSetupDebug();

		if (knockVal > quietKnock)
		{
			//valid knock, engage debounce/mute function
			sleepPiezoSensors(sleepPiezo1Start, mutePiezoSensors, 100);
			blinkLEDs(500, 500, 3);
			//if we have set a password, record rest of knocks and compare to PW;
			if (knockPWSet) {
				//user's pattern gets recorded in this boolean returning function
				if (recordKnocking(userTimeBetween, knockVal))
				{
					//playbackKnock(userTimeBetween, maxPWLength);
					if (compareKnockTimings(userTimeBetween, pwTimeBetweenKnock, maxPWLength))
					{
						//match: unlock!

						//\\//Serial.println("UNLOCKED!");

						knockMode = false;
						knockPWSet = false;
						unlockBox = true;
						sleepPiezoSensors(sleepPiezo1Start, mutePiezoSensors, 3000);
						sleepCapSensors(sleepCapStart, muteCapSensors, 3000);
					}
					else
					{
						clearPW(userTimeBetween, maxPWLength);

						//\\//Serial.println("Confirmed failed attempt");

						playTone(249, 1000);
					}

					//\\//Serial.println("Timing evaluation complete");

					//user's pattern gets compared against created PW
				}
				else
				{	//This means the user's attempt failed
					//Here we don't want to count failed attempts
					// as there could be accidental knock
					//if they forget pw, create a function to detect if all
					//cap sensors are pressed to exit this loop.
					clearPW(userTimeBetween, maxPWLength);

					//\\//Serial.println("Failed Attempt at PW");

				}

			}
			else
			{	
				//process starts here for setting up password
				//record password
				
				knockPWSet = recordKnocking(pwTimeBetweenKnock, knockVal);
				//if true, password is acceptable
				if (knockPWSet)
				{
					playbackKnock(pwTimeBetweenKnock, maxPWLength);
					
					//playTone(850, 1000);
					blinkLEDs(400, 100, 2);
					sleepSwitchSensor(sleepSwitchStart, muteSwitch, 1000);
					sleepCapSensors(sleepCapStart, muteCapSensors, 1000);
					sleepPiezoSensors(sleepPiezo1Start, mutePiezoSensors, 1000);
				}
				else {
					knockMode = false;

					//\\//Serial.println("Knock Mode Disengaged");

					//cancel mode altogether;
					unlockBox = true;
				}
			}	
		}

		//if they dont get that first knock

		//\\//Serial.print(knockPWSet);
		//\\//Serial.print("\t");
		//\\//Serial.print(millis());
		//\\//Serial.print("\t");
		//\\//Serial.println(setKnockPWStart);

		if (!(knockPWSet) && (millis() - setKnockPWStart >= setKnockPWTimeOut))
		{	

			//\\//Serial.println("no knocks attempted");

			unlockBox = true;
			knockMode = false;
		}
		//Three Button Override
		if (cs1_lvl + cs2_lvl + cs3_lvl > overallTouchThreshold)
		{
			if (cs1_lvl > crossKeyThreshold && cs2_lvl > crossKeyThreshold && cs3_lvl > crossKeyThreshold)
			{
				blinkLEDs(500, 500, 2);
				knockMode = false;
				knockPWSet = false;
				unlockBox = true;
				sleepPiezoSensors(sleepPiezo1Start, mutePiezoSensors, 3000);
				sleepCapSensors(sleepCapStart, muteCapSensors, 3000);
			}
		}
		if (switchVal == LOW && locked)
		{
			unlockBox = false;
			knockMode = true;
		}
		////\\//Serial.println(millis());
		updateAllLEDs();
		updateSpeaker();
	}
	
	//Three Button Override
	if (cs1_lvl + cs2_lvl + cs3_lvl > overallTouchThreshold)
	{
		if (cs1_lvl > crossKeyThreshold && cs2_lvl > crossKeyThreshold && cs3_lvl > crossKeyThreshold)
		{	
			
			//\\//Serial.println("All 3 cap buttons pressed");
			
			blinkLEDs(500, 500, 2);
			knockMode = false;
			knockPWSet = false;
			unlockBox = true;
			sleepPiezoSensors(sleepPiezo1Start, mutePiezoSensors, 3000);
			sleepCapSensors(sleepCapStart, muteCapSensors, 3000);
		}
	}
	
	//\\//Serial.println("End playKnockMode(knockMode)");
	
	clearPW(pwTimeBetweenKnock, maxPWLength);
	clearPW(userTimeBetween, maxPWLength);
	updateAllLEDs();
	updateSpeaker();
	if (unlockBox)
	{
		return true;
	}
	else
		return false;
}

void clearPW(unsigned long* timeArray, const int&maxSize) {

	//\\//Serial.print("Array b4 clear: ");

	for (int i = 0; i < maxSize; i++)
	{		
		
		//\\//Serial.print(timeArray[i]);
		//\\//Serial.print("\t");
		 
		timeArray[i] = 0;
	}

	//\\//Serial.print("\n");
	//\\//Serial.print("Cleared Array:  ");

	for (int i = 0; i < maxSize; i++)
	{
		//\\//Serial.print(timeArray[i]);
		//\\//Serial.print("\t");
	}
	//\\//Serial.print("\n");

}

boolean recordKnocking(unsigned long *timeArray, int& currentKnockValue) {
	boolean recordKnock = true;
	boolean recordSuccess = false;
	int numberOfKnocks = 0;
	unsigned long recentKnockTime = millis();

	//\\//Serial.print("K: ");
	//\\//Serial.print(currentKnockValue);
	//\\//Serial.print("\t ");
	//\\//Serial.println(millis());

	numberOfKnocks++;

	do
	{
		//the first time around currentKnockvalue is not being read due to sleepSensor;
		currentKnockValue = listenKnockOrIgnore(mainMidPiezo, leftPiezo, rightPiezo, mutePiezoSensors, sleepPiezo1Start, 100);
		//so this is 0 until sleep expires

		if (currentKnockValue > quietKnock)
		{
			
			//\\//Serial.print("K: ");
		 	//\\//Serial.print(currentKnockValue);
		 	//\\//Serial.print("\t ");
		 	//\\//Serial.print(millis());

			//\\//Serial.print("\t Gap: ");
			
			timeArray[numberOfKnocks - 1] = millis() - recentKnockTime;
			
			//\\//Serial.println(timeArray[numberOfKnocks - 1]);
			
			numberOfKnocks++;
			recentKnockTime = millis();
			sleepPiezoSensors(sleepPiezo1Start, mutePiezoSensors, 100);
		}
		if (millis() - recentKnockTime > knockRecordTimeOut)
		{
			if (numberOfKnocks == 1) {
			
			//\\//Serial.println("Invalid Knock PW: one knock");
			
				recordSuccess = false;
				recordKnock = false;

			}
			else
			{
				recordSuccess = true;
				recordKnock = false;
				////\\//Serial.print(numberOfKnocks);
				////\\//Serial.println(" knocks Password");
				//password recording ends here
			}
		}
		updateAllLEDs();
		updateSpeaker();
	} while (recordKnock && numberOfKnocks < maxPWLength);
	
	//\\//Serial.print("Recorded knocks: ");
	//\\//Serial.println(numberOfKnocks);
	
	return recordSuccess;

}

void playbackKnock(const unsigned long* timeArray, const int& maxPWLen) {
	int knocks = 0;
	int j = 0;
	unsigned long startKnockDelay = 0;
	boolean toggleOnce = false;
	
	for (int i = 0; i < maxPWLen; i++)
	{
		//\\//Serial.print(timeArray[i]);
		//\\//Serial.print("\t");
	}
	//\\//Serial.print("\n");
	
	for (int i = 0; i < maxPWLen; i++)
	{
		if (timeArray[i] > 0) {
			++knocks;
		}
	}
	
	//\\//Serial.print("playback knocks: ");
	//\\//Serial.println(knocks + 1);
	//\\//Serial.print("\n");

	startKnockDelay = millis();
	while (knocks > 0)
	{
		if (millis() - startKnockDelay < timeArray[j])
		{
			if (!toggleOnce)
			{
				//truncation is okay
				redLEDt(timeArray[j] - 75);
				yellowLEDt(timeArray[j] - 75);
				greenLEDt(timeArray[j] - 75);
				playTone(349,timeArray[j] - 75);
				toggleOnce = true;
				
				//\\//Serial.print("Tap\t");
				
			}

		}
		else
		{
			
		 	//\\//Serial.print(timeArray[j]);
		 	//\\//Serial.print("\t toggleOnce: ");
		 	//\\//Serial.print(toggleOnce);
		 	//\\//Serial.print("\tknocks: ");
		 	//\\//Serial.print(knocks);
		 	//\\//Serial.print("\t j: ");
		 	//\\//Serial.println(j);
			
		 	toggleOnce = false;
			--knocks;
			startKnockDelay = millis();
			++j;

			if (knocks == 0)
			{	

				delay(500);
				tone(speaker, 849, 50);
				digitalWrite(redLED, HIGH);
				digitalWrite(yellowLED, HIGH);
				digitalWrite(greenLED, HIGH);
				delay(50);
				noTone(speaker);
				digitalWrite(redLED, LOW);
				digitalWrite(yellowLED, LOW);
				digitalWrite(greenLED, LOW);
				delay(100);
				tone(speaker, 849, 50);
				digitalWrite(redLED, HIGH);
				digitalWrite(yellowLED, HIGH);
				digitalWrite(greenLED, HIGH);
				delay(50);
				noTone(speaker);
				digitalWrite(redLED, LOW);
				digitalWrite(yellowLED, LOW);
				digitalWrite(greenLED, LOW);
				delay(100);
				//redLEDt(1000);
				
				//\\//Serial.print("Tap\t");
				//\\//Serial.print(timeArray[j]);
				//\\//Serial.print("\t toggleOnce: ");
				//\\//Serial.print(toggleOnce);
				//\\//Serial.print("\tknocks: ");
				//\\//Serial.print(knocks);
				//\\//Serial.print("\t j: ");
				//\\//Serial.println(j);
				
			}
		}
		updateAllLEDs();
		updateSpeaker();
	}

	//\\//Serial.println("End while(knocks > 0)");

}

boolean compareKnockTimings(unsigned long *userKnockTiming, unsigned long *pwKnockTiming, const int& maxSize) {
	int userCount = 0;
	int pwCount = 0;

	for (int i = 0; i < maxSize; i++) {
		if (userKnockTiming[i] > 0)
		{
			
			//\\//Serial.print("us: ");
			//\\//Serial.print(i);
			//\\//Serial.print(userKnockTiming[i]);
			
			userCount++;
		}
		if (pwKnockTiming[i] > 0)
		{
			
			//\\//Serial.print("\t pw: ");
			//\\//Serial.print(i);
			//\\//Serial.print(pwKnockTiming[i]);
			
			pwCount++;
		}
	
	//\\//Serial.print("\n");
	
	}

	
	//\\//Serial.print("User: ");
	//\\//Serial.print(userCount + 1);
	//\\//Serial.print("Pw: ");
	//\\//Serial.println(pwCount + 1);
	

	if (userCount != pwCount)
	{
		
		//\\//Serial.println("Number of Knocks Mismatch");
		
		return false;
	}
	else
	{	
		
		//\\//Serial.println("Count matches!");
		unsigned long upLim = 0;
		unsigned long lowLim = 0;
		double pwKnockDec = 0;
		double inaccuracyP = 1.0 - ((double)knockPWAccuracy / 100.0);
		boolean falseDetected = false;
		//pwknock timing is the time between knocks
		// the smaller the number the earlier
		// the larger the number the later
		for (int i = 0; i < maxSize; i++) {

			pwKnockDec = (double)pwKnockTiming[i];

			//truncation happens here
			lowLim = pwKnockDec - (inaccuracyP * pwKnockDec);
			upLim = pwKnockDec + (inaccuracyP * pwKnockDec);
			
			//\\//Serial.print("[lowLim: ");
			//\\//Serial.print(lowLim);
			//\\//Serial.print("] [user: ");
			//\\//Serial.print(userKnockTiming[i]);
			//\\//Serial.print("] ");
		 	//\\//Serial.print("[upLim: ");
		 	//\\//Serial.print(upLim);
			

			if (userKnockTiming[i] >= lowLim && userKnockTiming[i] <= upLim)
			{		
				
				//\\//Serial.println("]\t -->accurate");
				
			}
			else
			{	
				
				//\\//Serial.println("]\t -->inaccurate");
				
				falseDetected = true;
			}
		}
		if (falseDetected) {
			return false;
		}

		return true;
	}
}
// GAME FUNCTIONS / GAME METHODS
boolean playGameMode(){
	
	//\\//Serial.println("playGameMode()");
	
	boolean runMode = true;
	boolean gameComplete = false;
	while (runMode) {
		
		//\\//Serial.println("runMode");
		
		do {
			printSetupDebug();
			generateTouchPW();
			playBackGamePW();
			gameComplete = matchGamePassword();
		} while ( gameComplete == false);
		
		//\\//Serial.println("gameComplete == false");
		
		if (gameComplete) {
			runMode = false;
		}
		if (switchVal == LOW)
		{	
			
			//\\//Serial.print("alarmGame \t");
			
			soundAlarm();
			runMode = false;
			return false;
		}
		updateAllLEDs();
		updateSpeaker();
	}
	return true;
}

boolean generateTouchPW() {
	//generates a 1-5 length password with a keypress length and time between press.
	gamePWLength = random(3, 6);//3-5

	for (int i = 0; i < maxPWLength; i++)
	{
		keyPressDelay[i] = 0;
		buttonPressSequence[i] = 0;
		keyPressTime[i] = 0;
	}
	for (int i = 0; i < gamePWLength; i++)
	{
		keyPressDelay[i] = 100; //* random(1, 2);
		keyPressTime[i] = 900; //* random(1, 2);
		buttonPressSequence[i] = random(11, 14);//11-13
		
		//\\//Serial.print(buttonPressSequence[i]);
		//\\//Serial.print("\t");
		//\\//Serial.print(keyPressTime[i]);
		//\\//Serial.print("\t");
		//\\//Serial.println(keyPressDelay[i]);
		
	}

}

void playBackGamePW() {

	cutSignals();
	int i = 0;
	boolean playToneNDelayPeriod = false;
	currentTime = millis();
	unsigned long playStartTime = currentTime;
	unsigned long elapsedPlayback = 0;

	unsigned long intervalDuration = 0;
	
	//\\//Serial.print("\n");
	
	while (i < gamePWLength)
	{
		////\\//Serial.println(currentTime);
		currentTime = millis();
		elapsedPlayback = currentTime - playStartTime;

		if (!playToneNDelayPeriod) {
			intervalDuration = keyPressTime[i] + keyPressDelay[i];
			
			//\\//Serial.print("intervalDuration: ");
			//\\//Serial.print(intervalDuration);
			//\\//Serial.print("\t keyPressTime[");
			//\\//Serial.print(i);
			//\\//Serial.print("]: ");
			//\\//Serial.print(keyPressTime[i]);
			//\\//Serial.print("\t keyPressDelay[");
			//\\//Serial.print(i);
			//\\//Serial.print("]: ");
			//\\//Serial.println(keyPressDelay[i]);
			
			switch (buttonPressSequence[i])
			{
			case 13: redLEDt(keyPressTime[i]);//left button
				playTone(208, keyPressTime[i]);
				break;
			case 12: yellowLEDt(keyPressTime[i]);//middle button
				playTone(277, keyPressTime[i]);
				break;
			case 11: greenLEDt(keyPressTime[i]);//right button
				playTone(330, keyPressTime[i]);
				break;
			default:
				
				//\\//Serial.println("OUTSIDE RANGE OF LED/BUTTONS");
				
				break;
			}
			playToneNDelayPeriod = true;
		}
		else
		{
			if (elapsedPlayback > keyPressTime[i] && elapsedPlayback > intervalDuration)
			{
				//cutSignals();
				
				//\\//Serial.print("delayPeriod: ");
				//\\//Serial.println(currentTime);
				
			}
			if (elapsedPlayback >= intervalDuration)
			{
				++i;
				playToneNDelayPeriod = false;
				playStartTime = currentTime;
				if (i > gamePWLength) {
					playTone(0,100);
				}
				//cutSignals();
			}
		}

		if (switchVal == LOW)
		{	
			
			//\\//Serial.print("alarmGame \t");
			
			soundAlarm();
		}

		updateAllLEDs();
		updateSpeaker();
	}
	
	//\\//Serial.print("\n");
	
}

//#5bb. 
boolean matchGamePassword() {
	boolean keyIsPressed = false;
	boolean gameWon = false;
	boolean runMode = true;

	int i = 0;
	int ledValue = 0;

	playTone(0, 100);
	
	//\\//Serial.println(gamePWLength);
	//\\//Serial.println("matchGameWhile");
	while (runMode && i < gamePWLength)
	{	

		knockVal = listenKnockOrIgnore(mainMidPiezo, leftPiezo, rightPiezo, mutePiezoSensors, sleepPiezo1Start, sleepPiezoDuration);
		senseCapsOrIgnore(cs1_lvl, cs2_lvl, cs3_lvl, mutePiezoSensors, sleepCapStart, sleepCapDuration);
		switchVal = readOrIgnoreSwitch(switchPin, muteSwitch, sleepSwitchStart, sleepSwitchDuration);

		if (cs1_lvl + cs2_lvl + cs3_lvl > overallTouchThreshold)
		{
			if (!keyIsPressed)
			{
				if (cs1_lvl > crossKeyThreshold) {
					//\\//Serial.print("Left bttn ");
					redLEDt(1000);
					playTone(208, 1000);
					ledValue = redLED;
				}
				if (cs2_lvl > crossKeyThreshold) {
					//\\//Serial.print("Mid bttn ");
					yellowLEDt(1000);
					playTone(277, 1000);
					ledValue = yellowLED;
				}
				if (cs3_lvl > crossKeyThreshold) {
					//\\//Serial.print("Right bttn ");
					greenLEDt(1000);
					playTone(330, 1000);
					ledValue = greenLED;
				}
				keyIsPressed = true;
				//debounce capacitive sensor issue
				sleepCapSensors(sleepCapStart, muteCapSensors, 100);

			}
			else { //keep playing noise for enjoyment
				if (keyIsPressed) {
					if (cs1_lvl > crossKeyThreshold && keyIsPressed) {
						redLEDt(1000);
						//playTone(208, 1000);
					}
					if (cs2_lvl > crossKeyThreshold) {
						yellowLEDt(1000);
						//playTone(277, 1000);
					}
					if (cs3_lvl > crossKeyThreshold) {
						greenLEDt(1000);
						//playTone(330, 1000);
					}
				}
			}
		}
		else
		{	//signals level have changed
			if (keyIsPressed) {
				keyIsPressed = false;
				if (ledValue == buttonPressSequence[i])
				{	
					//\\//Serial.print(ledValue);
					//\\//Serial.print(" : ");
					//\\//Serial.println(buttonPressSequence[i]);
					i++;
				}
				else {
					gameWon = false;
					//\\//Serial.println(" wrong key");
					delay(500);
					tone(speaker, 100, 250);
					digitalWrite(redLED, HIGH);
					digitalWrite(yellowLED, HIGH);
					digitalWrite(greenLED, HIGH);
					delay(250);
					noTone(speaker);
					digitalWrite(redLED, LOW);
					digitalWrite(yellowLED, LOW);
					digitalWrite(greenLED, LOW);
					delay(250);
					tone(speaker, 100, 250);
					digitalWrite(redLED, HIGH);
					digitalWrite(yellowLED, HIGH);
					digitalWrite(greenLED, HIGH);
					delay(250);
					noTone(speaker);
					digitalWrite(redLED, LOW);
					digitalWrite(yellowLED, LOW);
					digitalWrite(greenLED, LOW);
					delay(250);
					return false;
				}
			}
			//noTone(speaker);
		}

		if (i + 1 == gamePWLength)
		{
			gameWon = true;
		}
		//noTone(speaker);

		if (switchVal == LOW)
		{	
			runMode = false;
			gameMode = false;
		}
		updateAllLEDs();
		updateSpeaker();
	}//end while;
	if (gameWon)
	{
		return true;
	}
	else
	{	
		playTone(100, 500);
		return false;
	}

}
//END GAME FUNCTIONS / GAME METHODS

//LOCK METHOD / LOCK FUNCTION
void lock() {
	//#2.
	sleepCapSensors(sleepCapStart,muteCapSensors,3000);
	sleepPiezoSensors(sleepPiezo1Start, mutePiezoSensors, 3000);
	sleepSwitchSensor(sleepSwitchStart, muteSwitch, 2000);
	servo.write(90);

	//\\//Serial.println("LidClosed, Servo Locking Latch");

	locked = true;
	beginClosedLidTimer = false;
}
//UNLOCK METHOD / UNLOCK FUNCTION
void unlock() {
	restartMode();
	locked = false;
	sleepSwitchSensor(sleepSwitchStart, muteSwitch, 2000);
	sleepCapSensors(sleepCapStart, muteCapSensors, 3000);
	sleepPiezoSensors(sleepPiezo1Start, mutePiezoSensors, 3000);
	servo.write(0);
	
	//\\//Serial.println("Servo unlocking latch");
	
	//cutSignals();
}

//RESTART GAME MODE METHOD / RESTART GAME MODE FUNCTION
void restartMode() {
	isModeSet = false;
	gameMode = false;
	knockMode = false;
	startedModeSelection = false;
	beginClosedLidTimer = false;
	playAlarm = false;
}

//call this function to lock the box latch, also muteSensor input to stop motor disturbing mainMidPiezo
void sleepPiezoSensors(unsigned long& sleepStrt, boolean& muteSensorBool, const unsigned long& sleepDur) {

	 	//\\//Serial.print("Sensor mute for:");
	//\\//Serial.print(sleepDur);
	//\\//Serial.println("ms.");

	muteSensorBool = true;
	sleepStrt = millis();
	sleepPiezoDuration = sleepDur;
}

void sleepCapSensors(unsigned long& sleepStrt, boolean& muteSensorBool, const unsigned long& sleepDur) {

	 	//\\//Serial.print("Sensor mute for:");
	//\\//Serial.print(sleepDur);
	//\\//Serial.println("ms.");

	muteSensorBool = true;
	sleepStrt = millis();
	sleepCapDuration = sleepDur;
}
void sleepSwitchSensor(unsigned long& sleepStrt, boolean& muteSensorBool, const unsigned long& sleepDur) {
	muteSwitch = true;
	sleepStrt = millis();
	sleepSwitchDuration = sleepDur;
}

//SENSING FUNCTIONS
//this function will return the mainMidPiezo sensor reading depending if there is a sleep in effect (0) or not.
int listenKnockOrIgnore(const byte & mainPiezoPin,const byte& leftPiezoPin, const byte&rightPiezoPin, boolean & muteStatus, const unsigned long & sleepStart, const unsigned long & pNumsleepDur)
{
	if (!muteStatus)
	{
		return analogRead(mainPiezoPin);
	}
	if (millis() - sleepStart > pNumsleepDur)
	{
		muteStatus = false;
	}

	return 0;
}
//this method will assign capacitor readings depending if there is a sleep in effect (0) or not.
void senseCapsOrIgnore(long & cs1lvl, long & cs2lvl, long & cs3lvl, boolean & muteStatus, const unsigned long & sleepStart, const unsigned long & sleepDur)
{	
	if (!muteStatus)
	{	
		cs1lvl = cs1.capacitiveSensor(30);
		cs2lvl = cs2.capacitiveSensor(30);
		cs3lvl = cs3.capacitiveSensor(30);

	}
	else
	{
		cs1lvl = 0;
		cs2lvl = 0;
		cs3lvl = 0;
	}
	if (millis() - sleepStart > sleepDur)
	{
		muteStatus = false;
	}


}
int readOrIgnoreSwitch(const int&switchPin, boolean& muteSwtch, const unsigned long& sleepSwtchStart, const unsigned long& dur)
{
	if (!muteSwtch)
	{
		return digitalRead(switchPin);
	}
	if (millis() - sleepSwtchStart > dur)
	{
		muteSwtch = false;
	}
	return HIGH;
}


//LED FUNCTION/METHODS
void blinkLEDs(const int& timeOn, const int& timeOff, const int & numberOfTimes) {
	blinkStartTime = currentTime;
	oneBlinkInterval = (unsigned long)(timeOn + timeOff);
	blinkTOn = timeOn;
	blinkTOff = timeOff;
	blinkLightsStatus = true;
	blinkTimes = numberOfTimes;
}
void redLEDOn() {
	redState = HIGH;
}
void yellowLEDOn() {
	yellowState = HIGH;
}
void greenLEDOn() {
	greenState = HIGH;
}

void redLEDt(const unsigned long& duration) {
	redOnly = true;
	redDuration = duration;
	redLEDStart = millis();
}
void yellowLEDt(const unsigned long& duration) {
	yellowOnly = true;
	yellowDuration = duration;
	yellowLEDStart = millis();
}
void greenLEDt(const unsigned long& duration) {
	greenOnly = true;
	greenDuration = duration;
	greenLEDStart = millis();
}
void updateAllLEDs() {
	currentTime = millis();
	if (redOnly)
	{
		////\\//Serial.print(millis()-redLEDStart);
		////\\//Serial.print("\t");
		////\\//Serial.print(currentTime);
		////\\//Serial.print("\t");
		////\\//Serial.println(redLEDStart);
		if (currentTime - redLEDStart <= redDuration)
		{
			redState = HIGH;
		}
		else
		{
			redOnly = false;
			redState = LOW;
		}
	}
	if (yellowOnly)
	{
		if (currentTime - yellowLEDStart <= yellowDuration)
		{
			yellowState = HIGH;
		}
		else
		{
			yellowOnly = false;
			yellowState = LOW;
		}
	}
	if (greenOnly)
	{
		if (currentTime - greenLEDStart <= greenDuration)
		{
			greenState = HIGH;
		}
		else
		{
			greenOnly = false;
			greenState = LOW;
		}
	}
	if (blinkLightsStatus)
	{
		blinkElapsed = currentTime - blinkStartTime;
		////\\//Serial.print(allLEDsElapsed);
		////\\//Serial.print("\t");
		////\\//Serial.print(currentTime);
		////\\//Serial.print("\t");
		////\\//Serial.println(previousAllLED);
		if (blinkTimes != 0)
		{
			if (blinkElapsed < oneBlinkInterval)
			{
				if (blinkElapsed < blinkTOn)
				{
					redState = HIGH;
					yellowState = HIGH;
					greenState = HIGH;
				}
				else
				{
					redState = LOW;
					yellowState = LOW;
					greenState = LOW;
				}
			}
			else
			{
				blinkStartTime = currentTime;
				--blinkTimes;

				//\\//Serial.print("Blinks left: ");
				//\\//Serial.println(blinkTimes);

			}
		}
		else
		{
			blinkLightsStatus = false;
		}
	}

	digitalWrite(redLED, redState);
	digitalWrite(yellowLED, yellowState);
	digitalWrite(greenLED, greenState);
}
//END LED FUNCTION/METHOD SECTION//

//AUDIO FUNCTIONS / AUDIO METHODS
void updateSpeaker() {
	currentTime = millis();
	if (chirpHighStatus)
	{
		chirpHighElapsed = millis() - startChirpHigh;
		if (chirpHighElapsed < chirpHighDuration)
		{
			if (chirpHighElapsed < 120)
			{
				playTone(349, 120);
			}
			else if (chirpHighElapsed > 125 && chirpHighElapsed < 245)
			{
				playTone(698, 120);
			}
			else if (chirpHighElapsed > 250 && chirpHighElapsed < 370)
			{
				playTone(998, 120);

			}
			else if (chirpHighElapsed > 375 && chirpHighElapsed < 495)
			{
				playTone(2093, 120);
			}
			else
			{
				currentPitch = 0;
			}
		}
		else
		{
			chirpHighStatus = false;
			currentPitch = 0;
		}
	}
	if (chirpLowStatus) {
		chirpLowElapsed = millis() - startChirpLow;
		if (chirpLowElapsed < chirpLowDuration)
		{
			if (chirpLowElapsed < 120)
			{

				playTone(350, 120);
			}
			else if (chirpLowElapsed > 125 && chirpLowElapsed < 245)
			{
				playTone(200, 120);
			}
			else if (chirpLowElapsed > 250 && chirpLowElapsed < 370)
			{
				playTone(300, 120);
			}
			else if (chirpLowElapsed > 375 && chirpLowElapsed < 495)
			{
				playTone(190, 120);
			}
			else
			{
				currentPitch = 0;
			}
		}
		else
		{
			currentPitch = 0;
			chirpLowStatus = false;
		}
	}

	if (timedTonePlay)
	{
		if (millis() - startToneTime < toneDuration && currentPitch > 0) {
			tone(speaker, currentPitch);
		}
		else
		{
			timedTonePlay = false;
			noTone(speaker);
		}
	}

	if (playAlarm)
	{
		tone(speaker, 50);
		//NOTE: playAlarm becomes false by calling disableAlarm();
	}
}
void disableAlarm() {
	playAlarm = false;
	playTone(0, 50);
}
void soundAlarm() {
	playAlarm = true;
}
void playTone(const int& pitch, const unsigned long& duration) {
	if (pitch != previousPitch)
	{	
		noTone(speaker);
		previousPitch = pitch;
	}
	startToneTime = millis();
	toneDuration = duration;
	timedTonePlay = true;
	currentPitch = pitch;
}
void chirpLow() {
	//F D C B
	chirpLowStatus = true;
	startChirpLow = millis();
}
void chirpHigh() {
	chirpHighStatus = true;
	startChirpHigh = millis();
}
void cutSignals() {
	toneDuration = 0;
	currentPitch = 0;
	chirpLowStatus = false;
	chirpHighStatus = false;
	greenOnly = false;
	redOnly = false;
	yellowOnly = false;
	redState = LOW;
	greenState = LOW;
	yellowState = LOW;
	blinkLightsStatus = false;
	noTone(speaker);
}
// END AUDIO

//DEBUG FUNCTIONS / DEBUG METHODS
void printSetupDebug() {

	//\\//Serial.print("\n");
	//\\//Serial.print("button:");
	//\\//Serial.print("\t");
	//\\//Serial.print("locked:");
	//\\//Serial.print("\t");
	//\\//Serial.print("game:");
	//\\//Serial.print("\t");
	//\\//Serial.print("knock:");
	//\\//Serial.print("\t");
	//\\//Serial.print("cs1:");
	//\\//Serial.print("\t");
	//\\//Serial.print("cs2:");
	//\\//Serial.print("\t");
	//\\//Serial.print("cs3:");
	//\\//Serial.print("\t");
	//\\//Serial.print("piezo:");
	//\\//Serial.print("\n");

	//\\//Serial.print("-------");
	//\\//Serial.print("\t");
	//\\//Serial.print("-------");
	//\\//Serial.print("\t");
	//\\//Serial.print("-------");
	//\\//Serial.print("\t");
	//\\//Serial.print("-------");
	//\\//Serial.print("\t");
	//\\//Serial.print("-------");
	//\\//Serial.print("\t");
	//\\//Serial.print("-------");
	//\\//Serial.print("\t");
	//\\//Serial.print("-------");
	//\\//Serial.print("\t");
	//\\//Serial.print("-------");
	//\\//Serial.print("\n");

	//\\//Serial.print(switchVal);
	//\\//Serial.print("\t");
	//\\//Serial.print(locked);
	//\\//Serial.print("\t");
	//\\//Serial.print(gameMode);
	//\\//Serial.print("\t");
	//\\//Serial.print(knockMode);
	//\\//Serial.print("\t");
	//\\//Serial.print(cs1_lvl);
	//\\//Serial.print("\t");
	//\\//Serial.print(cs2_lvl);
	//\\//Serial.print("\t");
	//\\//Serial.print(cs3_lvl);
	//\\//Serial.print("\t");
	//\\//Serial.print(knockVal);
	//\\//Serial.print("\n");

}
void printBooleanDebug() {

	//\\//Serial.print("\n");
	//\\//Serial.print("blink:");
	//\\//Serial.print("\t");
	//\\//Serial.print("plAlarm:");
	//\\//Serial.print("\t");
	//\\//Serial.print("T_tone:");
	//\\//Serial.print("\t");
	//\\//Serial.print("chLow:");
	//\\//Serial.print("\t");
	//\\//Serial.print("chHigh:");
	//\\//Serial.print("\t");
	//\\//Serial.print("locked:");
	//\\//Serial.print("\t");
	//\\//Serial.print("bLidT:");
	//\\//Serial.print("\t");
	//\\//Serial.print("start:");
	//\\//Serial.print("\t");
	//\\//Serial.print("mode:");
	//\\//Serial.print("\t");
	//\\//Serial.print("knoM:");
	//\\//Serial.print("\t");
	//\\//Serial.print("gaM:");
	//\\//Serial.print("\n");

	for (int i = 0; i < 11; i++)
	{
		//\\//Serial.print("-------");
		//\\//Serial.print("\t");
	}

	//\\//Serial.print("\n");
	//\\//Serial.print(blinkLightsStatus);
	//\\//Serial.print("\t");
	//\\//Serial.print(playAlarm);
	//\\//Serial.print("\t");
	//\\//Serial.print(timedTonePlay);
	//\\//Serial.print("\t");
	//\\//Serial.print(chirpLowStatus);
	//\\//Serial.print("\t");
	//\\//Serial.print(chirpHighStatus);
	//\\//Serial.print("\t");
	//\\//Serial.print(locked);
	//\\//Serial.print("\t");
	//\\//Serial.print(beginClosedLidTimer);
	//\\//Serial.print("\t");
	//\\//Serial.print(startedModeSelection);
	//\\//Serial.print("\t");
	//\\//Serial.print(isModeSet);
	//\\//Serial.print("\t");
	//\\//Serial.print(knockMode);
	//\\//Serial.print("\t");
	//\\//Serial.print(gameMode);
	//\\//Serial.print("\n");

}
//END DEBUG FUNCTIONS

