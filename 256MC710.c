/**********************************************************************
 *                                                                     *
 *                        Software License Agreement                   *
 *                                                                     *
 *    The software supplied herewith by Microchip Technology           *
 *    Incorporated (the "Company") for its dsPIC controller            *
 *    is intended and supplied to you, the Company's customer,         *
 *    for use solely and exclusively on Microchip dsPIC                *
 *    products. The software is owned by the Company and/or its        *
 *    supplier, and is protected under applicable copyright laws. All  *
 *    rights are reserved. Any use in violation of the foregoing       *
 *    restrictions may subject the user to criminal sanctions under    *
 *    applicable laws, as well as to civil liability for the breach of *
 *    the terms and conditions of this license.                        *
 *                                                                     *
 *    THIS SOFTWARE IS PROVIDED IN AN "AS IS" CONDITION.  NO           *
 *    WARRANTIES, WHETHER EXPRESS, IMPLIED OR STATUTORY, INCLUDING,    *
 *    BUT NOT LIMITED TO, IMPLIED WARRANTIES OF MERCHANTABILITY AND    *
 *    FITNESS FOR A PARTICULAR PURPOSE APPLY TO THIS SOFTWARE. THE     *
 *    COMPANY SHALL NOT, IN ANY CIRCUMSTANCES, BE LIABLE FOR SPECIAL,  *
 *    INCIDENTAL OR CONSEQUENTIAL DAMAGES, FOR ANY REASON WHATSOEVER.  *
 *                                                                     *
  **********************************************************************/


/*=============================================================================
    Initialize ports for p33FJ256MC710
=============================================================================*/

#include "general.h"

void SetupPorts( void )
{


// ============= Port A ==============

// RA0/TMS                                               I Led and header to ext board
// RA1/TCK                                               I Led and header to ext board
// RA2                                                   I Led and header to ext board
// RA3                                                   I Led and header to ext board
// RA4/TDI                                               I Led and header to ext board
// RA5/TD0                                               I Led and header to ext board
// RA6                                                   I Led and header to ext board
// RA7                                                   I Led and header to ext board
// RA8    absent
// RA9    												 O Not connected
// RA10   												 O Not connected
// RA11   absent
// RA12   absent
// RA13   absent
// RA14    												 O Not connected
// RA15    												 O Not connected
//                                                      1111111110000000b


    TRISA = 0xFF80;
    LATA  = 0x0000;
        AD1PCFGHbits.PCFG23=1;  // Select Port pin RA7 in Digital mode, (MUX)
        AD1PCFGHbits.PCFG22=1;  // Select Port pin RA6 in Digital mode, (MUX)

// ============= Port B ==============

// RB0    PGD/EMUD/AN0/CN2      AI  Phase1 I 
// RB1    PGC/EMUC/AN1/CN3      AI  Phase2 I 
// RB2    AN2/SS1/LVDIN/CN4     AI  Phase3 I
// RB3    AN3/INDX/CN5          I   QEI Index

// RB4    AN4/QEA/CN6           I   QEI A
// RB5    AN5/QEB/CN7           I   QEI B
// RB6    AN6/PGC 	            A?I PGC  
// RB7    AN7/PGD               A?I PGD 

// RB8    AN8                   A?I Not connected
// RB9    AN9                   A?I Not connected
// RB10   AN10                  A?I Not connected
// RB11   AN11                  A?I Not connected

// RB12   AN12                  A?I Not connected
// RB13   AN13                  A?I Not connected
// RB14   AN14                  A?I Not connected
// RB15   AN15/OCFB/CN12        A?I Not connected
//                                              
    LATB  = 0x0000;
    TRISB = 0xFFFF;

// ============= Port C ==============

// RC0       absent				I
// RC1    RC1					I   
// RC2    RC2					I
// RC3    RC3                	I   

// RC4    RC4					I
//        absent
//        absent
//        absent

//        absent
//        absent
//        absent
//        absent

// RC12   OSC1
// RC13   EMUD1/SOSC2/CN1           EMUD1
// RC14   EMUC1/SOSC1/T1CK/CN0      EMUC1
// RC15   OSC2/CLKO

    LATC  = 0x0000;
    TRISC = 0xFFFF;

// ============= Port D ============== F30

// RD0    EMUC2/OC1             I
// RD1    EMUD2/OC2             I      
// RD2    OC3                   I
// RD3    OC4                   I

// RD4    OC5/CN13              I
// RD5    OC6/CN14              I	Button P1
// RD6    OC7/CN15              I   Button 1 (S3) (Active low)      
// RD7    OC8/CN16/UPDN         I   Button 2 (S4) (Active low)

// RD8    IC1                   O   Desat reset
// RD9    IC2                   I   CAP2	-unused V2 remove
// RD10   IC3                   I   CAP3	-unused V2 remove
// RD11   IC4                   I   FLT input , V2:change to output enable, OR-ed with FLT in hardware

// RD12   IC5					I	
// RD13   IC6/CN19              I   Button 4 (S4) (Active low)

// RD14   IC7/CN20				I
// RD15   IC8/CN21				I

    LATD  = 0x0000;
    TRISD = 0xFEFF;

// ============= Port E ==============

// RE0    PWM1L                 O   Phase1 L
// RE1    PWM1H                 O   Phase1 H
// RE2    PWM2L                 O   Phase2 L
// RE3    PWM2H                 O   Phase2 H

// RE4    PWM3L                 O   Phase3 L
// RE5    PWM3H                 O   Phase3 H
// RE6    PWM4L                 O   Phase4 L
// RE7    PWM4H                 O   Phase4 H

// RE8    FLTA/INT1             I   Fault'
// RE9    FLTB/INT2             I   
//        absent
//        absent

//        absent
//        absent
//        absent
//        absent

    LATE  = 0x0000;
    TRISE = 0xFF00;

// ============= Port F ==============

// RF0     		                I   CNint
// RF1    	                 	I   CNint
// RF2    RX					O				
// RF3    TX

// RF4    
// RF5    
// RF6    
// RF7    

// RF8    
//        absent
//        absent
//        absent

//        absent
//        absent
//        absent
//        absent

    LATF  = 0x0000;
    TRISF = 0xFFFF;

// ============= Port G ==============

// RG0    C2RX                  O   485 RE'
// RG1    C2TX                  O   485 DE
// RG2    SCL                   I/O SCL
// RG3    SDA                   I/O SDA

//        absent
//        absent
//        absent
//        absent
//        absent
//        absent
//        absent
//        absent

//        absent
//        absent
//        absent
//        absent

    LATG  = 0x0000;
    TRISG = 0x0100;

}

