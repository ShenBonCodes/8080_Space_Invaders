#ifndef EMULATOR_H
#define EMULATOR_H

#define opcodeCount 0x100

 typedef struct ConditionCodes {    
    uint8_t    z:1;     // state of sign bit
    uint8_t    s:1;     // state of zero bit
    uint8_t    p:1;     // parity bit
    uint8_t    cy:1;    // carry bit 
    uint8_t    ac:1;    // state of auxilary carry bit
    uint8_t    pad:3;   // state of parity bit
   } ConditionCodes;    

   typedef struct State8080 {    
    uint8_t    a;    
    uint8_t    b;    
    uint8_t    c;    
    uint8_t    d;    
    uint8_t    e;    
    uint8_t    h;    
    uint8_t    l;    
    uint16_t   sp;    
    uint16_t   pc;    
    uint8_t    *memory;    
    struct ConditionCodes cc;    
    uint8_t    int_enable;    
   } State8080;

#endif
