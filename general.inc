;;--------------------- Common stuff for ASM routines ---------------------

.include "p33FJ256MC710.inc"


;;--------------------- Test stuff for simulation ---------------------

        .extern _uTestPos
;;        .equ SIMU,1   ; simulate encoder
        .equ TEST_CUR_MODEL,1   ; testing for current model

