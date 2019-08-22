// AC CONTROL/DRIVER BOARD 21
#include "uart.h"

#define FP 30000000
#define BAUDRATE 115200
#define BRGVAL ((FP/BAUDRATE)/16)-1

void ShowMenu(void);
void ShowConfig();
void u16x_to_str(char *str, unsigned val, unsigned char digits);
void u16_to_str(char *str, unsigned val, unsigned char digits);
void int16_to_str(char *str, int val, unsigned char digits);
int TransmitString(const char* str);
char IntToCharHex(unsigned int i);
void FetchRTData(void);
void StopAllMotorTests(void);

extern void InitPIStruct(void);
extern void EESaveValues(void);
extern void InitializeThrottleAndCurrentVariables(void);
extern void TurnOffADAndPWM();
extern void InitADAndPWM();
extern void InitQEI();

volatile UARTCommand myUARTCommand = {0,0,{0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0},0};

extern volatile int IqRefRef;
extern volatile int IdRefRef;
extern volatile int captureData;
extern volatile int dataCaptureIndex;
extern volatile int currentMaxIterationsBeforeZeroCrossing;

extern volatile int PDCtemp;
extern long int vRef1AvgSum;
extern long int vRef2AvgSum;
extern volatile int vRef1Avg;
extern volatile int vRef2Avg;
extern int ADCBuffer[4]  __attribute__((space(dma)));   
//extern unsigned int ADCBuffer0;
//extern unsigned int ADCBuffer1;
//extern unsigned int ADCBuffer2;
//extern unsigned int ADCBuffer3;


extern volatile int vRef1;
extern volatile int vRef2;
extern volatile int maxRPS_times16;
extern volatile unsigned int faultBits;
extern volatile SavedValuesStruct savedValues;
extern volatile SavedValuesStruct2 savedValues2;
extern unsigned int revCounterMax;
extern volatile unsigned int poscnt;	
extern volatile unsigned int counter10k;
extern volatile unsigned int counter1k;
extern volatile piType myPI;
extern volatile scanType scan;
extern volatile rotorTestType myRotorTest;
extern volatile angleOffsetTestType myAngleOffsetTest;
extern volatile motorSaliencyTestType myMotorSaliencyTest;
extern volatile int bigArray1[];

volatile int readyToDisplayBigArrays = 0;
volatile dataStream myDataStream;

volatile int timeSinceLastCarriageReturn = 0;
volatile char newChar = 0;
volatile int echoNewChar = 0;
volatile dataStream myDataStream;
char intString[] = "xxxxxxxxxx";
					//      0         1         2         3         4
					//      01234567890123456789012345678901234567890	
char showConfigString[] = "xxx";
void InitUART1(){
	U1MODEbits.STSEL = 0; 	// 1-Stop bit
	U1MODEbits.PDSEL = 0; 	// No Parity, 8-Data bits
	U1MODEbits.ABAUD = 0;	// Auto-Baud disabled
	U1MODEbits.BRGH = 0; 	// Standard-Speed mode
	U1BRG = BRGVAL; 		// Baud Rate 
//	U1STAbits.UTXISEL0 = 0; // Interrupt after one TX character is transmitted
//	U1STAbits.UTXISEL1 = 0;
//	IEC0bits.U1TXIE = 1; 	// Enable UART TX interrupt

	U1STAbits.URXISEL0 = 0; // Interrupt after one TX character is transmitted
	U1STAbits.URXISEL1 = 0;
	IEC0bits.U1RXIE = 1; 	// Enable UART TX interrupt
	U1MODEbits.UARTEN = 1; 	// Enable UART
	U1STAbits.UTXEN = 1;
//	Delay(100u);			// TODO JH include this line
}

void __attribute__((__interrupt__, auto_psv)) _U1RXInterrupt(void) {
	IFS0bits.U1RXIF = 0;  // clear the interrupt.
	echoNewChar = 1;
	newChar = U1RXREG;		// get the character that caused the interrupt.


	if (myUARTCommand.complete == 1) {	// just ignore everything until the command is processed.
		return;
	}

	if (newChar == 0x0d) {	// carriage return.
//		if (counter10k - timeSinceLastCarriageReturn < 2000) return;
//		timeSinceLastCarriageReturn = counter10k;
		myUARTCommand.complete = 1;
		myUARTCommand.string[myUARTCommand.i] = 0;  // instead of placing a carriage return, place a 0 to null terminate the string.
		return;
	}
	if (myUARTCommand.i >= MAX_COMMAND_LENGTH) {  // the command was too long.  It's just garbage anyway, so start over.
		//myUARTCommand.complete = 0;  // It can't make it here unless myUARTCommand.complete == 0 anyway.
		myUARTCommand.i = 0;	// just clear the array, and start over.
		myUARTCommand.string[0] = 0;
//		myUARTCommand.number = 0;  // This is done in "ProcessCommand", so you don't need to do it here.
		return;
	}
	myUARTCommand.string[myUARTCommand.i] = newChar; // save the character that caused the interrupt!
	myUARTCommand.i++;
}

// process the command, and reset UARTCommandPtr back to zero.
// myUARTCommand is of the form XXXXXXXXX YYYYY<enter>
void ProcessCommand(void) {
	static int i = 0;
	if (echoNewChar) {
//		StopAllMotorTests();		// also, stop the motor tests.
		while (echoNewChar) {
			if (U1STAbits.UTXBF == 0) { // TransmitReady();
				U1TXREG = newChar; 	// SendCharacter(newChar);
				if (newChar == 0x0d) {
					while (1) { 
						if (U1STAbits.UTXBF == 0) { // TransmitReady();
							U1TXREG = 0x0a; 	// SendCharacter(line feed);
							break;
						}
					}
				}
				echoNewChar = 0;
			}
		}
	}
	else {
		if (myUARTCommand.complete != 1) {	// if the command isn't yet complete, don't try to process it!  Maybe someone is only half-way done with their command.  Ex:  "sav".  Process "sav"?  No!  wait until they type "save<cr>"
			return;
		}
		myUARTCommand.number = 0;	
		for (i = 0; myUARTCommand.string[i] != 0; i++) {
			if (myUARTCommand.string[i] == ' ') {
				myUARTCommand.number = atoi((char *)&myUARTCommand.string[i+1]);
				myUARTCommand.string[i] = 0;  // null terminate the text portion.			
				break;
			}
		}
		if (!strcmp((const char *)&myUARTCommand.string[0], "save")) {
//			TurnOffADAndPWM(); // TODO JH 
//			EESaveValues();		//JH
//			InitADAndPWM();		//JH
		}


		else if (!strcmp((const char *)&myUARTCommand.string[0], "scan")) {
			scan.DoScanMe=1;
		}
		else if (!strcmp((const char *)&myUARTCommand.string[0], "trigger")) {
			scan.trigger=1;
		}

		else if (!strcmp((const char *)&myUARTCommand.string[0], "motor-type")) {
			if (myUARTCommand.number > 0 && myUARTCommand.number < 5) {
//				TurnOffADAndPWM(); // TODO JH
				savedValues.motorType = myUARTCommand.number;
//JH TODO				InitADAndPWM();
//JH TODO				InitQEI();
			}
		}
		// Let's say you typed the command "kp 1035".  The following would have happened:
		// myUARTCommand.string[] would contain only the text portion of the command, and is terminated with a 0.  string[] = {'p',0,?,?,?,?,?,?,?,?,?,?,?,...}
		// Also, myUARTCommand.number = the number argument after the command. So, number = 1035.
		else if (!strcmp((const char *)&myUARTCommand.string[0], "kp")) {
			if (myUARTCommand.number <= 32767u && myUARTCommand.number > 0) {
				savedValues.Kp = (int)(myUARTCommand.number); 
				InitPIStruct();
			}
		}
		else if (!strcmp((const char *)&myUARTCommand.string[0], "ki")){ 
			if (myUARTCommand.number <= 32767u && myUARTCommand.number > 0) {
				savedValues.Ki = (int)(myUARTCommand.number); 
				InitPIStruct();
			}
		}
		else if (!strcmp((const char *)&myUARTCommand.string[0], "angle-offset")){ 
			if (myUARTCommand.number <= 511 && myUARTCommand.number >= 0) {
				savedValues2.angleOffset = (unsigned int)(myUARTCommand.number); 	// this one is the extra for displaying on the screen.
				myAngleOffsetTest.currentAngleOffset = savedValues2.angleOffset;  	// this is the working copy.
			}
		}
		else if (!strcmp((const char *)&myUARTCommand.string[0], "saliency")){ 
			if (myUARTCommand.number <= 1024 && myUARTCommand.number >= 0) {  // 
				savedValues2.KArrayIndex = (unsigned int)(myUARTCommand.number); 	// this one is the extra for displaying on the screen.
				myMotorSaliencyTest.KArrayIndex = savedValues2.KArrayIndex;  	// this is the working copy.
			}
		}		
		else if (!strcmp((const char *)&myUARTCommand.string[0], "current-sensor-amps-per-volt")) {  // 
			if (myUARTCommand.number <= 480 && myUARTCommand.number > 0) {
				savedValues.currentSensorAmpsPerVolt = (int)(myUARTCommand.number); 
				InitializeThrottleAndCurrentVariables();
			}	
		}
		else if (!strcmp((const char *)&myUARTCommand.string[0], "max-regen-position")) { 
			if (myUARTCommand.number <= 1023u && myUARTCommand.number > 0) {
				savedValues.maxRegenPosition = (int)(myUARTCommand.number); 
			}	
		}
		else if (!strcmp((const char *)&myUARTCommand.string[0], "min-regen-position")) { 
			if (myUARTCommand.number <= 1023u && myUARTCommand.number > 0) {
				savedValues.minRegenPosition = (int)(myUARTCommand.number); 
			}	
		}
		else if (!strcmp((const char *)&myUARTCommand.string[0], "min-throttle-position")) { 
			if (myUARTCommand.number <= 1023u && myUARTCommand.number > 0) {
				savedValues.minThrottlePosition = (int)(myUARTCommand.number); 
			}
		}
		else if (!strcmp((const char *)&myUARTCommand.string[0], "max-throttle-position")) { 
			if (myUARTCommand.number <= 1023u && myUARTCommand.number > 0) {
				savedValues.maxThrottlePosition = (int)(myUARTCommand.number); 
			}	
		}
		else if (!strcmp((const char *)&myUARTCommand.string[0], "fault-throttle-position")) { 
			if (myUARTCommand.number <= 1023u && myUARTCommand.number > 0) {
				savedValues.throttleFaultPosition = (int)(myUARTCommand.number); 
			}	
		}
		else if (!strcmp((const char *)&myUARTCommand.string[0], "max-battery-amps")) { 
			if (myUARTCommand.number <= 9999 && myUARTCommand.number > 0) {
				savedValues.maxBatteryAmps = (int)(myUARTCommand.number); 
				InitializeThrottleAndCurrentVariables();
			}
		}
		else if (!strcmp((const char *)&myUARTCommand.string[0], "max-battery-amps-regen")) { 
			if (myUARTCommand.number <= 9999 && myUARTCommand.number > 0) {
				savedValues.maxBatteryAmpsRegen = (int)(myUARTCommand.number); 
				InitializeThrottleAndCurrentVariables();
			}
		}
		else if (!strcmp((const char *)&myUARTCommand.string[0], "max-motor-amps")) { 
			if (myUARTCommand.number <= 999 && myUARTCommand.number > 0) {
				savedValues.maxMotorAmps = (int)(myUARTCommand.number); 
				InitializeThrottleAndCurrentVariables();
			}
		}
		else if (!strcmp((const char *)&myUARTCommand.string[0], "max-motor-amps-regen")) { 
			if (myUARTCommand.number <= 999 && myUARTCommand.number > 0) {
				savedValues.maxMotorAmpsRegen = (int)(myUARTCommand.number); 
				InitializeThrottleAndCurrentVariables();
			}
		}
		else if (!strcmp((const char *)&myUARTCommand.string[0], "precharge-time")) { 
			if (myUARTCommand.number <= 9999 && myUARTCommand.number > 0) {
				savedValues.prechargeTime = (int)(myUARTCommand.number); 
			}
		}
		// NOW WE ARE ON SavedValues2...
		else if (!strcmp((const char *)&myUARTCommand.string[0], "rtc")) { 
			if (myUARTCommand.number <= ROTOR_TIME_CONSTANT_ARRAY_SIZE+5 && myUARTCommand.number >= 5) {
 				myRotorTest.timeConstantIndex = savedValues2.rotorTimeConstantIndex = (int)(myUARTCommand.number-5);
			}
		}
		else if (!strcmp((const char *)&myUARTCommand.string[0], "pole-pairs")) {
			if (myUARTCommand.number <= 999 && myUARTCommand.number >= 1) {
				savedValues2.numberOfPolePairs = (int)(myUARTCommand.number); 
			}
		}
		else if (!strcmp((const char *)&myUARTCommand.string[0], "max-rpm")) { 
			if (myUARTCommand.number <= 32767 && myUARTCommand.number > 0) {
				savedValues2.maxRPM = (int)(myUARTCommand.number); 
				maxRPS_times16 = (((long)savedValues2.maxRPM) << 2) / 15;  // 4/15 to convert to rps_times16
			}
		}
		else if (!strcmp((const char *)&myUARTCommand.string[0], "throttle-type")) { // 0 means hall effect throttle, or maxOHms to 0 Ohms. 1 means 0 Ohms to maxOhms throttle 
			if (myUARTCommand.number <= 1) {
				savedValues2.throttleType = (int)(myUARTCommand.number); 
			}
		}
		else if (!strcmp((const char *)&myUARTCommand.string[0], "encoder-ticks")) {
			if (myUARTCommand.number <= 5000u && myUARTCommand.number >= 16) {
				savedValues2.encoderTicks = (int)(myUARTCommand.number); 
				revCounterMax = (unsigned)(160000L / (4*savedValues2.encoderTicks));  // 4* because I'm doing 4 times resolution for the encoder. 160,000 because revolutions per 16 seconds is computed as:  16*10,000*poscnt * rev/(maxPosCnt*revcounter*(16sec)
				// revCounterMax may only be of use for the induction motor.
			}
		}
		else if (!strcmp((const char *)&myUARTCommand.string[0], "pi-ratio")) {
			if (myUARTCommand.number < 1000 && myUARTCommand.number >= 50) {
				myPI.ratioKpKi = (int)(myUARTCommand.number); 
			}
		}
		else if (!strcmp((const char *)&myUARTCommand.string[0], "pi")) {
			currentMaxIterationsBeforeZeroCrossing = 20;
			myPI.testRunning = 1;
			myPI.testFinished = 0;
			myPI.zeroCrossingIndex = -1;
			myPI.Kp = myPI.ratioKpKi;
			myPI.Ki = 1;
//			myPI.Kp = savedValues.Kp;
//			myPI.Ki = savedValues.Ki;
		}

		else if (!strcmp((const char *)&myUARTCommand.string[0], "pt")) {
			currentMaxIterationsBeforeZeroCrossing = 20;
			myPI.testRunningT = 1;
			myPI.testFinished = 0;
		}

		else if (!strcmp((const char *)&myUARTCommand.string[0], "p2")) {
			myPI.testRunning2 = 1;
		}
		else if (!strcmp((const char *)&myUARTCommand.string[0], "rrt")) {
			if (savedValues.motorType == 1) {		
				myRotorTest.startTime = counter10k;
				myRotorTest.timeConstantIndex = 0;	// always start at zero, and then it will increment up to around 145, giving each rotorTimeConstant candidate 5 seconds to spin the motor the best it can.
				myRotorTest.testRunning = 1;
				myRotorTest.testFinished = 0;
				myRotorTest.maxTestSpeed = 0;
				myRotorTest.bestTimeConstantIndex = 0;
			}
			else {
				TransmitString("#I,Your motor type is currently set to permanent magnet.  This test is for an AC induction motor,!\r\n");
				TransmitString("#I,To change your motor to AC induction, the command is 'motor-type 1',!\r\n");
			}
		}
		else if (!strcmp((const char *)&myUARTCommand.string[0], "run-angle-offset-test")) {
			if (savedValues.motorType >= 2) {
				if (myUARTCommand.number < 512) {			// angleOffset is normalized to something in [0,511]
					myAngleOffsetTest.startTime = counter10k;
					myAngleOffsetTest.currentAngleOffset = myUARTCommand.number;	// it will increment up to 511, giving each angle candidate some time.
					myAngleOffsetTest.testRunning = 1;
					myAngleOffsetTest.testFinished = 0;
					myAngleOffsetTest.testFailed = 1;
//					myAngleOffsetTest.maxTestSpeed = 0;
//					myAngleOffsetTest.bestAngleOffset = 0;
				}
			}
			else {
				TransmitString("#I,Your motor type is AC induction.  This test is for a permanent maget AC motor,!\r\n");
				TransmitString("#I,To change your motor to permanent maget_ the command is 'motor-type 2',!\r\n");
			}
		}
		else if (!strcmp((const char *)&myUARTCommand.string[0], "run-saliency-test")) {
			if (savedValues.motorType >= 2) {
				if (myUARTCommand.number < 1024) {  // What percent of the currentRadius should Id be (but negative)?
					myMotorSaliencyTest.startTime = myMotorSaliencyTest.elapsedTime = counter10k;
					myMotorSaliencyTest.KArrayIndex = myUARTCommand.number;	
					myMotorSaliencyTest.testRunning = 1;
					myMotorSaliencyTest.testFinished = 0;
					myMotorSaliencyTest.testFailed = 1;
//					myMotorSaliencyTest.maxTestSpeed = 0;
//					myMotorSaliencyTest.bestKArrayIndex = 0;
				}
			}
			else {
				TransmitString("Your motor type is AC induction.  This test is for a permanent maget AC motor,!\r\n");
				TransmitString("To change your motor to permanent maget, the command is 'motor-type 2' or 'motor-type 3',!\r\n");
			}
		}


		else if ((!strcmp((const char *)&myUARTCommand.string[0], "config")) || (!strcmp((const char *)&myUARTCommand.string[0], "settings"))|| (!strcmp((const char *)&myUARTCommand.string[0], "s"))) {
			ShowConfig();
		}

		else if (!strcmp((const char *)&myUARTCommand.string[0], "off")) {
			if (myRotorTest.testRunning) {  // Stop the rotor test if it was running, and just keep the best value of the rotor time constant that you had found up to this point.
				savedValues2.rotorTimeConstantIndex = myRotorTest.bestTimeConstantIndex;
				myRotorTest.timeConstantIndex = savedValues2.rotorTimeConstantIndex;

				myRotorTest.testRunning = 0;
				myRotorTest.testFinished = 1;
//				currentRadiusRefRef = 0;
			}
			else if (myPI.testRunning) { // Stop the PI test if it was running.
				currentMaxIterationsBeforeZeroCrossing = 20;
				InitPIStruct();
				myPI.testFailed = 1;
				myPI.testFinished = 1;
//				currentRadiusRefRef = 0;
			}
			if (myAngleOffsetTest.testRunning) {  // 
				myAngleOffsetTest.testRunning = 0;
				myAngleOffsetTest.testFinished = 1;
			}
			myDataStream.period = 0;  // Stop the data stream if it was running.  I already do this any time a key is hit, so this is redundant.
			// if the PI test is running, terminate it.
			// if the rotor test is running, stop and use the current best rotorTimeConstant that has been found so far.
			// if the angle offset test is running, stop and use the current best angle offset that has been found so far.

			ShowMenu();
		}

		else if (!strcmp((const char*)&myUARTCommand.string[0], "swap-ab")) {
			savedValues2.swapAB = (myUARTCommand.number & 1);
			QEICONbits.SWPAB = savedValues2.swapAB;
		}
		else if (!strcmp((const char*)&myUARTCommand.string[0], "2")) {
			if (myAngleOffsetTest.currentAngleOffset < 511-5) {
				myAngleOffsetTest.currentAngleOffset+=5;
				savedValues2.angleOffset = myAngleOffsetTest.currentAngleOffset;  	// this is the working copy.
			}
		}
		else if (!strcmp((const char*)&myUARTCommand.string[0], "1")) {
			if (myAngleOffsetTest.currentAngleOffset >= 5) {
				myAngleOffsetTest.currentAngleOffset-=5;
				savedValues2.angleOffset = myAngleOffsetTest.currentAngleOffset;  	// this is the working copy.
			}
		}
//		else if (!strcmp((const char*)&myUARTCommand.string[0], "4")) {
//		}
//		else if (!strcmp((const char*)&myUARTCommand.string[0], "3")) {
//		}
		else if (!strcmp((const char*)&myUARTCommand.string[0], "c")) {
			dataCaptureIndex = 0;
			captureData = 1;
		}

//		else if (!strcmp((const char *)&myUARTCommand.string[0], "?")) {  // show the valid list of commands
//			TransmitString("List of valid commands:\r\n");
//			TransmitString("save\r\n");
//			TransmitString("motor-type xxx (rangle 1-4)\r\n");
//			TransmitString("kp xxx (range 0-32767)\r\n");
//			TransmitString("ki xxx (range 0-32767)\r\n");
//			TransmitString("current-sensor-amps-per-volt xxx (range 0-480)\r\n");
//			TransmitString("max-regen-position xxx (range 0-1023)\r\n");
//			TransmitString("min-regen-position xxx (range 0-1023)\r\n");
//			TransmitString("min-throttle-position xxx (range 0-1023)\r\n");
//			TransmitString("max-throttle-position xxx (range 0-1023)\r\n");
//			TransmitString("fault-throttle-position xxx (range 0-1023)\r\n");
//			TransmitString("max-battery-amps xxx (range 0-999)\r\n");
//			TransmitString("max-battery-amps-regen xxx (range 0-999)\r\n");
//			TransmitString("max-motor-amps xxx (range 0-999)\r\n");
//			TransmitString("max-motor-amps-regen xxx (range 0-999)\r\n");
//			TransmitString("precharge-time xxx (in tenths of a sec. range 0-9999)\r\n");
//			TransmitString("rotor-time-constant xxx (in millisec. range 0-150)\r\n");
//			TransmitString("pole-pairs xxx (range 0-999)\r\n");
//			TransmitString("max-rpm xxx (range 0-32767)\r\n");
//			TransmitString("throttle-type xxx (range 0-1)\r\n");
//			TransmitString("encoder-ticks xxx (range 64-5000)\r\n");
//			TransmitString("pi-ratio xxx (range 50-1000.  pi-ratio = Kp/Ki)\r\n");
//			TransmitString("angle-offset xxx (range 0-511)\r\n");
//			TransmitString("saliency xxx (range 0-1023)\r\n");
//			TransmitString("run-pi-test\r\n");
//			TransmitString("run-rotor-test\r\n");
//			TransmitString("run-angle-offset-test\r\n");
//			TransmitString("run-saliency-test\r\n");			
//			TransmitString("config\r\n");
//			TransmitString("data-stream-period xxx (range 0-32767)\r\n");
//			TransmitString("data\r\n");
//			TransmitString("stream-time xxx (range 0-1)\r\n");
//			TransmitString("stream-id xxx (range 0-1)\r\n");
//			TransmitString("stream-iq xxx (range 0-1)\r\n");
//			TransmitString("stream-idref xxx (range 0-1)\r\n");
//			TransmitString("stream-iqref xxx (range 0-1)\r\n");
//			TransmitString("stream-ia xxx (range 0-1)\r\n");
//			TransmitString("stream-ib xxx (range 0-1)\r\n");
//			TransmitString("stream-ic xxx (range 0-1)\r\n");
//			TransmitString("stream-percent-volts xxx (range 0-1)\r\n");
//			TransmitString("stream-battery-amps xxx (range 0-1)\r\n");
//			TransmitString("stream-raw-throttle xxx (range 0-1)\r\n");
//			TransmitString("stream-throttle xxx (range 0-1)\r\n");
//			TransmitString("stream-temperature xxx (range 0-1)\r\n");
//			TransmitString("stream-slip-speed xxx (range 0-1)\r\n");
//			TransmitString("stream-electrical-speed xxx (range 0-1)\r\n");
//			TransmitString("stream-mechanical-speed xxx (range 0-1)\r\n");
//			TransmitString("stream-poscnt xxx (range 0-1)\r\n");
//			TransmitString("off (this stops the data stream)\r\n");
//			TransmitString("swap-ab xxx (range 0-1)\r\n"); 
//		}
		else {
			TransmitString("#I,Invalid command.  Type '?' to see a valid list of commands.,!\r\n");
		}
	
		myUARTCommand.string[0] = 0; 	// clear the string.
		myUARTCommand.i = 0;
		myUARTCommand.number = 0;
		myUARTCommand.complete = 0;  // You processed that command.  Dump it!  Do this last.  The ISR will only run through if the command is NOT yet complete (in other words, if complete == 0). 
	}
}

void StopAllMotorTests() {
	myDataStream.period = 0;  	// stop the data stream during this test.
	myPI.testRunning = 0;	 	// stop the PI test if it's running
	myPI.testFailed = 1;	
	myPI.testFinished = 0;
	myRotorTest.testRunning = 0;	// stop the rotor time constant search if there was one.
	myRotorTest.testFinished = 0;
	myAngleOffsetTest.testRunning = 0;	// stop the permanent magnet angle offset search if there was one.
	myAngleOffsetTest.testFinished = 0;	
	myMotorSaliencyTest.testRunning = 0;	// stop the permanent magnet angle offset search if there was one.
	myMotorSaliencyTest.testFinished = 0;	
}

void ShowConfig() {
	// 0         1         2         3         4         5
	// 012345678901234567890123456789012345678901234567890123456789
	// motor-type=x\r\n
		strcpy(showConfigString,"#I,motor-type=x,!\r\n");
		u16_to_str(&showConfigString[11], savedValues.motorType, 1);	
		TransmitString(showConfigString);
	// 0         1         2         3         4         5
	// 012345678901234567890123456789012345678901234567890123456789
	// kp=xxxxx ki=xxxxx,!\r\n
		strcpy(showConfigString,"#I,kp=xxxxx ki=xxxxx,!\r\n");
		u16_to_str(&showConfigString[3], savedValues.Kp, 5);	
		u16_to_str(&showConfigString[12], savedValues.Ki, 5);
		TransmitString(showConfigString);
	// 0         1         2         3         4         5
	// 012345678901234567890123456789012345678901234567890123456789
	// current-sensor-amps-per-volt=xxxx,!\r\n;
		strcpy(showConfigString,"#I,current-sensor-amps-per-volt=xxxx,!\r\n");
		u16_to_str(&showConfigString[29], savedValues.currentSensorAmpsPerVolt, 4);
		TransmitString(showConfigString);
	// 0         1         2         3         4         5         6         7         8
	// 01234567890123456789012345678901234567890123456789012345678901234567890123456789012345
	// max-regen-position=xxxx,!\r\n
	// min-regen-position=xxxx,!\r\n 
		strcpy(showConfigString,"#I,max-regen-position=xxxx,!\r\n");
		u16_to_str(&showConfigString[19], savedValues.maxRegenPosition, 4);
		TransmitString(showConfigString);
		strcpy(showConfigString,"#I,min-regen-position=xxxx,!\r\n");
		u16_to_str(&showConfigString[19], savedValues.minRegenPosition, 4);
		TransmitString(showConfigString);
	// 0         1         2         3         4         5         6         7         8
	// 01234567890123456789012345678901234567890123456789012345678901234567890123456789012345
	// min-throttle-position=xxxx,!\r\n
	// max-throttle-position=xxxx,!\r\n 
	// fault-throttle-position=xxxx,!\r\n
		strcpy(showConfigString,"#I,min-throttle-position=xxxx,!\r\n");
		u16_to_str(&showConfigString[22], savedValues.minThrottlePosition, 4);
		TransmitString(showConfigString);

		strcpy(showConfigString,"#I,max-throttle-position=xxxx,!\r\n");
		u16_to_str(&showConfigString[22], savedValues.maxThrottlePosition, 4);
		TransmitString(showConfigString);

		strcpy(showConfigString,"#I,fault-throttle-position=xxxx,!\r\n");
		u16_to_str(&showConfigString[24], savedValues.throttleFaultPosition, 4);
		TransmitString(showConfigString);
	// 0         1         2         3         4         5         6         7         8
	// 01234567890123456789012345678901234567890123456789012345678901234567890123456789012345
	// max-battery-amps=xxxx amps,!\r\n
	// max-battery-amps-regen=xxxx amps,!\r\n
		strcpy(showConfigString,"#I,max-battery-amps=xxxz amps,!\r\n");
		u16_to_str(&showConfigString[17], savedValues.maxBatteryAmps, 4);
		TransmitString(showConfigString);

		strcpy(showConfigString,"#I,max-battery-amps-regen=xzxx amps,!\r\n");
		u16_to_str(&showConfigString[23], savedValues.maxBatteryAmpsRegen, 4);
		TransmitString(showConfigString);
	// 0         1         2         3         4         5
	// 012345678901234567890123456789012345678901234567890123456789
	// max-motor-amps=xxx amps,!\r\n
	// max-motor-amps-regen=xxx amps,!\r\n
		strcpy(showConfigString,"#I,max-motor-amps=xxx amps,!\r\n");
		u16_to_str(&showConfigString[15], savedValues.maxMotorAmps, 3);
		TransmitString(showConfigString);

		strcpy(showConfigString,"#I,max-motor-amps-regen=xxx amps,!\r\n");
		u16_to_str(&showConfigString[21], savedValues.maxMotorAmpsRegen, 3);
		TransmitString(showConfigString);
	// 0         1         2         3         4         5
	// 012345678901234567890123456789012345678901234567890123456789
	// precharge-time=xxxx tenths of a sec,!\r\n
		strcpy(showConfigString,"#I,precharge-time=xxxx tenths of a sec,!\r\n");
		u16_to_str(&showConfigString[15], savedValues.prechargeTime, 4);
		TransmitString(showConfigString);

		if (savedValues.motorType == 1) {
			// **NOW WE ARE IN SavedValues2**
			// 0         1         2         3         4         5
			// 012345678901234567890123456789012345678901234567890123456789
			// rotor-time-constant=xxx ms,!\r\n
			//
			strcpy(showConfigString,"#I,rotor-time-constant=xxx ms,!\r\n");
			u16_to_str(&showConfigString[20], savedValues2.rotorTimeConstantIndex+5, 3);  // for display purposes, add 5 so it's millisec.
			TransmitString(showConfigString);
		}
		else {
			// **NOW WE ARE IN SavedValues2**
			// 0         1         2         3         4         5
			// 012345678901234567890123456789012345678901234567890123456789
			// angle-offset=xxx,!\r\n
			//
			strcpy(showConfigString,"#I,angle-offset=xxx,!\r\n");
			u16_to_str(&showConfigString[13], savedValues2.angleOffset, 3);  // for display purposes, add 5 so it's millisec.
			TransmitString(showConfigString);			
			// 0         1         2         3         4         5
			// 012345678901234567890123456789012345678901234567890123456789
			// saliency=xxxx,!\r\n
			//
			strcpy(showConfigString,"#I,saliency=xxxx,!\r\n");
			u16_to_str(&showConfigString[9], savedValues2.KArrayIndex, 4);  // for display purposes, add 5 so it's millisec.
			TransmitString(showConfigString);			
		}

	// 0         1         2         3         4         5
	// 012345678901234567890123456789012345678901234567890123456789
	// pole-pairs=xxx,!\r\n
	//
		strcpy(showConfigString,"#I,pole-pairs=xxx,!\r\n");
		u16_to_str(&showConfigString[11], savedValues2.numberOfPolePairs, 3);  
		TransmitString(showConfigString);
	// 0         1         2         3         4         5
	// 012345678901234567890123456789012345678901234567890123456789
	// max-rpm=xxxxx rev/min,!\r\n
	//
		strcpy(showConfigString,"#I,max-rpm=xxxxx rev/min,!\r\n");
		u16_to_str(&showConfigString[8], savedValues2.maxRPM, 5);  
		TransmitString(showConfigString);
	// 0         1         2         3         4         5
	// 012345678901234567890123456789012345678901234567890123456789
	// throttle-type=x,!\r\n
	//
		strcpy(showConfigString,"#I,throttle-type=x,!\r\n");
		u16_to_str(&showConfigString[14], savedValues2.throttleType, 1);  
		TransmitString(showConfigString);
	// 0         1         2         3         4         5
	// 012345678901234567890123456789012345678901234567890123456789
	// encoder-ticks=xxxx ticks/rev,!\r\n
	//
		strcpy(showConfigString,"#I,encoder-ticks=xxxx ticks/rev,!\r\n");
		u16_to_str(&showConfigString[14], savedValues2.encoderTicks, 4);  
		TransmitString(showConfigString);
	// 0         1         2         3         4         5
	// 012345678901234567890123456789012345678901234567890123456789
	// pi-ratio=xxx,!\r\n
	//
		strcpy(showConfigString,"#I,pi-ratio=xxx,!\r\n");
		u16_to_str(&showConfigString[9], myPI.ratioKpKi, 3);  //
		TransmitString(showConfigString);
	// 0         1         2         3         4         5
	// 012345678901234567890123456789012345678901234567890123456789
	// raw-throttle=xxxx,!\r\n
	//
		strcpy(showConfigString,"#I,raw-throttle=xxxx,!\r\n");
		u16_to_str(&showConfigString[13],  ADCBuffer[0], 4);  //
		TransmitString(showConfigString);

		strcpy(showConfigString,"#I,Swap_AB=x,!\r\n");
		u16_to_str(&showConfigString[8], savedValues2.swapAB, 1);  // 
		TransmitString(showConfigString);

}

// Input is an integer from 0 to 15.  Output is a character in '0', '1', '2', ..., '9', 'a','b','c','d','e','f'
char IntToCharHex(unsigned int i) {
	if (i <= 9) {
		return ((unsigned char)(i + 48));
	}
	else {
		return ((unsigned char)(i + 55));
	}
}

void ShowMenu(void)
{
	TransmitString("#I,AC controller firmware_ver. 1.0,!\r\n");
}

// convert val to string (inside body of string) with specified number of digits
// do NOT terminate string
void u16_to_str(char *str, unsigned val, unsigned char digits)
{
	str = str + (digits - 1); // go from right to left.
	while (digits > 0) { // 
		*str = (unsigned char)(val % 10) + '0';
		val = val / 10;
		str--;
		digits--;
	}
}

// convert val to string (inside body of string) with specified number of digits (not counting the + or - sign).
// do NOT terminate string
// Ex:  -2345 should have length 4. It will be printed as -2345
// 2345 should also have length 4.  It will be printed as +2345.
// So, the first symbol is either '-' or '+'. 
void int16_to_str(char *str, int val, unsigned char digits)
{	
	if (val < 0) {
		str[0] = '-';
		val = -val;
	}
	else {
		str[0] = '+';
	}
	str = str + digits; // go from right to left.
	while (digits > 0) { // 
		*str = (unsigned char)(val % 10) + '0';
		val = val / 10;
		str--;
		digits--;
	}
}
// convert val to hex string (inside body of string) with specified number of digits
// do NOT terminate string
void u16x_to_str(char *str, unsigned val, unsigned char digits)
{
	unsigned char nibble;
	
	str = str + (digits - 1);
	while (digits > 0) {
		nibble = val & 0x000f;
		if (nibble >= 10) nibble = (nibble - 10) + 'A';
		else nibble = nibble + '0';
		*str = nibble;
		val = val >> 4;
		str--;
		digits--;
	}
}
int TransmitString(const char* str) {  // For echoing onto the display
	unsigned int i = 0;
//	unsigned int now = 0; JH
	
//	now = TMR5;	// timer 4 runs at 59KHz.  Timer5 is the high word of the 32 bit timer.  So, it updates about 1 time per second.
	while (1) {
		if (str[i] == 0) {
			return 1;
		}
		if (U1STAbits.UTXBF == 0) { // TransmitReady();
			U1TXREG = str[i]; 	// SendCharacter(str[i]);
			i++;
		}
//		if (TMR5 - now > 5000) { 	// 5 seconds
//			faultBits |= UART_FAULT;
//			return 0;
//		}
//		#ifndef DEBUG 
//			ClrWdt();
//		#endif
	}
}
void StreamData() {
//     Serial.println("#T,1,19,29,30,29,26,26,26,29,26,26,30,29,27,26,22,0,26,0,!");
	
//Start
	TransmitString("#T");

//1
	strcpy(showConfigString,",xxxxx");
	u16_to_str(&showConfigString[1], myDataStream.Id_times10, 5);
	TransmitString(showConfigString);

//2
	strcpy(showConfigString,",xxxxx");
	u16_to_str(&showConfigString[1], myDataStream.Iq_times10, 5);
	TransmitString(showConfigString);

//3
	strcpy(showConfigString,",xxxxx");
	u16_to_str(&showConfigString[1], myDataStream.IdRef_times10, 5);
	TransmitString(showConfigString);

//4
	strcpy(showConfigString,",xxxxx");
	u16_to_str(&showConfigString[1], myDataStream.IqRef_times10, 5);
	TransmitString(showConfigString);
				
//5
	strcpy(showConfigString,",xxxxx");
	u16_to_str(&showConfigString[1], myDataStream.batteryAmps_times10, 5);
	TransmitString(showConfigString);

//6
	strcpy(showConfigString,",xxxxx");
	u16_to_str((char *)&showConfigString[1], myDataStream.Ib_times10, 5);	 // ex: intString[] = "+087"
	TransmitString(showConfigString);

//7	
	strcpy(showConfigString,",xxxxx");
	u16_to_str((char *)&showConfigString[1], myDataStream.Ia_times10, 5);	 // ex: intString[] = "+087"
	TransmitString(showConfigString);

//8
	strcpy(showConfigString,",xxx ");
	u16_to_str(&showConfigString[1], myDataStream.percentOfVoltageDiskBeingUsed, 3);
	TransmitString(showConfigString);

//9
	strcpy(showConfigString,",xxxxx");
	u16_to_str(&showConfigString[1], myDataStream.rawThrottle, 5);
	TransmitString(showConfigString);

//10
	strcpy(showConfigString,",xxx");
	u16_to_str(&showConfigString[1], myDataStream.throttle, 3);
	TransmitString(showConfigString);

//11
	strcpy(showConfigString,",xxxx");
	u16_to_str((char *)&showConfigString[1], myDataStream.slipSpeedRPM, 4); // intString[] = "+0345".  Now, add a comma and null terminate it.
	TransmitString(showConfigString);

//12
	strcpy(showConfigString,",xxxxx");
	u16_to_str((char *)&showConfigString[1], myDataStream.electricalSpeedRPM, 5); // intString[] = "+03457".  Now, add a comma and null terminate it.
	TransmitString(showConfigString);

//13
	strcpy(showConfigString,",xxxxx");
	u16_to_str((char *)&showConfigString[1], myDataStream.mechanicalSpeedRPM, 5); // intString[] = "+03457".  Now, add a comma and null terminate it.
	TransmitString(showConfigString);

// 14
	strcpy(showConfigString,",xxxxx");
	u16_to_str((char *)&showConfigString[1], POSCNT, 5); // intString[] = "03457".  Now, add a comma and null terminate it.
	TransmitString(showConfigString);

// end
	TransmitString(",!\r\n");
}
void TransmitBigArrays() {
	int i = 0;
	if (readyToDisplayBigArrays) {
		readyToDisplayBigArrays = 0;
        TransmitString("#B");
		for (i = 0; i < 4094; i++) {
			int16_to_str((char *)&intString[0], bigArray1[i], 4);  // intString[] = "+0345".  Now, add a comma and null terminate it.
			intString[5] = ',';
			intString[6] = 0;	
			TransmitString((char *)&intString[0]);
			ClrWdt();
		}
		TransmitString(",!\r\n\r\n");
	}
}
