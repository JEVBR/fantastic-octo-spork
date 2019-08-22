#ifndef general_H
#define general_H

//--------------------- Common stuff for C routines ---------------------

#include <p33FJ256MC710.h>

#ifdef INITIALIZE
    // allocate storage
    #define EXTERN
#else
    #define EXTERN extern
#endif

typedef unsigned short WORD;
typedef unsigned char  BYTE;
typedef unsigned char  bool;
#define False  0
#define True   1

void DangIt(int i); 
void InitTimers();
void InitIORegisters(void);
void Delay(unsigned int);
void DelayTenthsSecond(int time);
void DelaySeconds(int time);
void InitADAndPWM();
void InitQEI();
void TurnOffADAndPWM();
void InitCNModule();
//void 3();
void InitPIStruct();
void InitPIStructForPITest();
void ClearAllFaults();  // clear the flip flop fault and the desat fault.
void ClearDesatFault();
void ClearFlipFlop();
void ComputePositionAndSpeed();
void SpaceVectorModulation();
void ClampVdVq();
void Delay1uS();
void GrabDataSnapshot();
void RAAAAR();
void ChargeUpCapacitorBank();
void UpdateCounter1k();


void ScanMe(long);
void RunPITest();
void RunPITest2();
void RunRotorTest();
void RunMotorSaliencyTest();
void RunAngleOffsetTest();
void RunFirstEncoderIndexPulseTest();

void SetupPorts( void );

void GetCurrentRadiusRefRef();
void UpdateDataStream();
void DisplayTestResults();
void CheckForFaults();
void DisplayFaultMessages();
void Setup();
void InitializeThrottleAndCurrentVariables();
void MoveToNextPIValues();

/******************************************************/
void __attribute__((__interrupt__)) _DMA0Interrupt(void);
void __attribute__((__interrupt__, auto_psv)) _U1RXInterrupt(void);
void __attribute__((__interrupt__, auto_psv)) _U1TXInterrupt(void);
void __attribute__((__interrupt__, auto_psv)) _FLTAInterrupt(void);
void __attribute__((__interrupt__, auto_psv)) _FLTBInterrupt(void);
void SetupDMA (void);
void SetupBoard( void );
void SetupPeripherals(void);


#endif      // end of general_H

