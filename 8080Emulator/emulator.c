#include <stdio.h>
#include <stdlib.h>
#include "emulator.h"

void Emulate8080(State8080* state);

const int opcodeSize[opcodeCount] = {
    1, 3, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 1, 1, 2, 1,
    1, 3, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 1, 1, 2, 1,
    1, 3, 3, 1, 1, 1, 2, 1, 1, 1, 3, 1, 1, 1, 2, 1,
    1, 3, 3, 1, 1, 1, 2, 1, 1, 1, 3, 1, 1, 1, 2, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 3, 3, 3, 1, 2, 1, 1, 1, 3, 3, 3, 3, 2, 1,
    1, 1, 3, 2, 3, 1, 2, 1, 1, 1, 3, 2, 3, 3, 2, 1,
    1, 1, 3, 1, 3, 1, 2, 1, 1, 1, 3, 1, 3, 3, 2, 1,
    1, 1, 3, 1, 3, 1, 2, 1, 1, 1, 3, 1, 3, 3, 2, 1
};

const int opcodeCycle[opcodeCount] = {
    4, 10, 7, 5, 5, 5, 7, 4, 4, 10, 7, 5, 5, 5, 7, 4,
    4, 10, 7, 5, 5, 5, 7, 4, 4, 10, 7, 5, 5, 5, 7, 4,
    4, 10, 16, 5, 5, 5, 7, 4, 4, 10, 16, 5, 5, 5, 7, 4,
    4, 10, 13, 5, 10, 10, 10, 4, 4, 10, 13, 5, 5, 5, 7, 4,
    5, 5, 5, 5, 5, 5, 7, 5, 5, 5, 5, 5, 5, 5, 7, 5,
    5, 5, 5, 5, 5, 5, 7, 5, 5, 5, 5, 5, 5, 5, 7, 5,
    5, 5, 5, 5, 5, 5, 7, 5, 5, 5, 5, 5, 5, 5, 7, 5,
    7, 7, 7, 7, 7, 7, 7, 7, 5, 5, 5, 5, 5, 5, 7, 5,
    4, 4, 4, 4, 4, 4, 7, 4, 4, 4, 4, 4, 4, 4, 7, 4,
    4, 4, 4, 4, 4, 4, 7, 4, 4, 4, 4, 4, 4, 4, 7, 4,
    4, 4, 4, 4, 4, 4, 7, 4, 4, 4, 4, 4, 4, 4, 7, 4,
    4, 4, 4, 4, 4, 4, 7, 4, 4, 4, 4, 4, 4, 4, 7, 4,
    5, 10, 10, 10, 11, 11, 7, 11, 5, 10, 10, 10, 11, 17, 7, 11,
    5, 10, 10, 10, 11, 11, 7, 11, 5, 10, 10, 10, 11, 17, 7, 11,
    5, 10, 10, 18, 11, 11, 7, 11, 5, 5, 10, 4, 11, 17, 7, 11,
    5, 10, 10, 4, 11, 11, 7, 11, 5, 5, 10, 4, 11, 17, 7, 11
};

int main(int argc, char** argv)
{
	FILE *file = fopen(argv[1], "rb");
	if (file == NULL)
	{
		printf("error: Couldn't open %s\n", argv[1]);    
          	exit(1);
	}
	fseek(file, 0L, SEEK_END);
	int fsize = ftell(file);
	fseek(file, 0L, SEEK_SET);

	unsigned char *buffer = malloc(fsize);

	fread(buffer, fsize, 1, file);
	fclose(file);

	int pc = 0; // program counter
	
	while (pc < fsize)
	{
		pc += Dissasembler8080p(buffer, pc);
	}
	
	return 0;
}

int parityCheck(uint8_t result)
{
	int oneCount = 0;
	int bit;

	for(int x = 0; x < 8; x++)
	{
		bit = (result) & (1<<(x));
		if (bit != 0)
			oneCount++;
	}

	if ((oneCount % 2) == 0) // even
		result = 1;

	return result;
}

void update_s_flag(State8080* state, uint8_t result) 
{	// set to 1 when bit 7 (the most significant bit or MSB) of the math instruction is set
	if ((result & 0x80)) // == 1
		state->cc.s = 1;
	else
		state->cc.s = 0;
}

void update_z_flag(State8080* state, uint8_t result) 
{	// set to 1 when the result is equal to zero
	if (result == 0)
		state->cc.z = 1;
	else
		state->cc.z = 0;
}

void update_p_flag(State8080* state, uint8_t result)
{	// set when the answer has even parity, clear when odd parity
	state->cc.p = parityCheck(result);
}

void update_ac_flag_add(State8080* state, uint8_t val1, uint8_t val2, int useCarry)
{
	int carry = useCarry ? state->cc.cy : 0;
	val1 = val1 & ~(0x10); // clears 5th bit
	uint8_t result = val1 + val2 + carry;

	state->cc.ac = result & 0x10 ? 1 : 0;
}

void update_ac_flag_sub(State8080* state, uint8_t val1, uint8_t val2, int useCarry)
{
	int carry = useCarry ? state->cc.cy : 0;
	val1 = val1 & ~(0x10); // clears 5th bit
	val2 = (uint8_t)~val2 + 1; // 2's complement
	uint8_t result = val1 + val2 + carry;

	state->cc.ac = result & 0x10 ? 1 : 0;
}

void update_c_flag_add(State8080* state, uint8_t val1, uint8_t val2, int useCarry)
{	// set to 1 when the instruction resulted in a carry out or borrow into the high order bit
	int carry = useCarry ? state->cc.cy : 0;
	uint16_t result = (uint16_t)val1 + (uint16_t)val2 + carry;

	if (result > 0xff) // if there is a carry bit
		state->cc.cy = 1;    
	else    
		state->cc.cy = 0;    
}

void update_c_flag_sub(State8080* state, uint8_t val1, uint8_t val2, int useCarry)
{	// set to 1 when the instruction resulted in a carry out or borrow into the high order bit
	int carry = useCarry ? state->cc.cy : 0;
	val2 = ~val2 + 1;
	uint16_t result = (uint16_t)val1 + (uint16_t)val2 + carry;

	if (result > 0xff) // if there is a carry bit
		state->cc.cy = 0;       
}

void movRegs(uint8_t* dest, uint8_t* src)
{
	*dest = *src;
}

void incRegOrMem(State8080* state, uint8_t* val1, uint8_t val2)
{	
	uint8_t result = (uint8_t)*val1 + val2;
	update_s_flag(state, result);
	update_z_flag(state, result);
	update_ac_flag_add(state, *val1, val2, 0);
	update_p_flag(state, result);
	*val1 = result;
}

void decRegOrMem(State8080* state, uint8_t* val1, uint8_t val2)
{	
	uint8_t result = (uint8_t)*val1 - val2;
	update_s_flag(state, result);
	update_z_flag(state, result);
	update_ac_flag_sub(state, *val1, val2, 0);
	update_p_flag(state, result);
	*val1 = result;
}

void ADD(State8080* state, uint8_t* val1, uint8_t val2)
{	
	uint8_t result = (uint8_t)*val1 + val2;
	update_s_flag(state, result);
	update_z_flag(state, result);
	update_ac_flag_add(state, *val1, val2, 0);
	update_p_flag(state, result);
	update_c_flag_add(state, *val1, val2, 0);
	*val1 = result;
}

void ADC(State8080* state, uint8_t* val1, uint8_t val2)
{	
	uint8_t result = (uint8_t)*val1 + val2 + state->c;

	update_s_flag(state, result);
	update_z_flag(state, result);
	update_ac_flag_add(state, *val1, val2, 1);
	update_p_flag(state, result);
	update_c_flag_add(state, *val1, val2, 1);

	*val1 = result;
}

void SUB(State8080* state, uint8_t* val1, uint8_t val2)
{	
	uint8_t result = *val1 + (uint8_t)(~val2 + 1);

	update_s_flag(state, result);
	update_z_flag(state, result);
	update_ac_flag_sub(state, *val1, val2, 0);
	update_p_flag(state, result);
	update_c_flag_sub(state, *val1, val2, 0);

	*val1 = result;
}

void SBB(State8080* state, uint8_t* val1, uint8_t val2)
{	
	val2 += state->cc.cy;
	uint8_t result = (uint8_t)*val1 + (uint8_t)(~val2 + 1);

	update_s_flag(state, result);
	update_z_flag(state, result);
	update_ac_flag_sub(state, *val1, val2, 0);
	update_p_flag(state, result);
	update_c_flag_sub(state, *val1, val2, 0);

	*val1 = result;
}

void ANA(State8080* state, uint8_t* val1, uint8_t val2)
{
	uint8_t result = *val1 & val2;

	update_s_flag(state, result);
	update_z_flag(state, result);
	update_p_flag(state, result);
	state->cc.ac = result | (1 << 3);
	state->cc.cy = 0;

	*val1 = result;
}

void XRA(State8080* state, uint8_t* val1, uint8_t val2)
{
	uint8_t result = *val1 ^ val2;

	update_s_flag(state, result);
	update_z_flag(state, result);
	update_p_flag(state, result);
	state->cc.cy = 0;

	*val1 = result;
}

void ORA(State8080* state, uint8_t* val1, uint8_t val2)
{
	uint8_t result = *val1 | val2;

	update_s_flag(state, result);
	update_z_flag(state, result);
	update_p_flag(state, result);
	state->cc.cy = 0;

	*val1 = result;
}

void CMP(State8080* state, uint8_t* val1, uint8_t val2)
{
	uint8_t result = *val1 - val2;

	update_s_flag(state, result);
	update_z_flag(state, result);
	update_p_flag(state, result);
	update_c_flag_sub(state, *val1, val2, 0);
	update_ac_flag_sub(state, *val1, val2, 0);
}

void doubleAdd(State8080* state, uint16_t val1, uint16_t val2)
{	// not using update_c_flag func as using 16 bit addition
	uint32_t result = (uint32_t)val1 + (uint32_t)val2;

	if (result > 0xffff)    
		state->cc.cy = 1;    
	else    
		state->cc.cy = 0;
}

uint16_t getRegPair(uint8_t msb, uint8_t lsb) // returns address created by register pair concatenation
{
	uint16_t addr = msb << 8 | lsb;
	return addr;
}

uint16_t getHLAddr(State8080* state) // returns address created by HL register pair concatenation
{
	uint16_t addr = state->h << 8 | state->l;
	return addr;
}

void Emulate8080(State8080* state)
{
	unsigned char* opcode = &state->memory[state->pc];
	int column = *opcode >> 4;
	int row = (uint8_t)*opcode;

	switch (*opcode) 
	{
		case 0x00:	// NOP	
		case 0x10:		
		case 0x20:		
		case 0x30:		
		case 0x08:		
		case 0x18:		
		case 0x28:		
		case 0x38:	
			break;

		/* LXI, d16, load 16-bit immediate data into rp (B, D, H, SP) */
		// little-endian, second byte is least significant 
		case 0x01:	state->c = opcode[1]; state->b = opcode[2]; 					break; 	// LXI B
		case 0x11:	state->e = opcode[1]; state->d = opcode[2];                	break; 	// LXI D, d16
		case 0x21:	state->l = opcode[1]; state->h = opcode[2];    				break;	// LXI H, d16
			
		case 0x31:	// LXI SP, d16
			// shift left 8 bits, then use OR operator to put add LSB to right hand side
			// because 0 | x = x (x is 0 or 1), so right 8 bits will just be LSB (least significant bits)
			uint16_t addr = opcode[2] << 8 | opcode[1];
			state->sp = addr;             
			break;
		
		/* STAX, store accumulator */
		case 0x02:	state->memory[getRegPair(state->b, state->c)] = state->a; 	break;	// STAX B
		case 0x12:	state->memory[getRegPair(state->d, state->e)] = state->a;	break;	// STAX D
			
		/* LDAX, load accumulator */
		case 0x0a:	state->a = state->memory[getRegPair(state->b, state->c)];	break;	// LDAX B
		case 0x1a:	state->a = state->memory[getRegPair(state->d, state->e)];	break;	// LDAX C
			 
		case 0x22:	// SHLD, Store H and L to memory
			uint16_t addr = opcode[1] << 8 | opcode[0];	
			state->memory[addr] = state->l;
			state->memory[(uint16_t)(addr + 1)] = state->h;     
			break; 

		case 0x2a:	// LHLD, Load H and L from
			uint16_t addr = opcode[1] << 8 | opcode[0];	
			state->l = state->memory[addr]; 
			state->h = state->memory[(uint16_t)(addr + 1)];
			break;

		case 0x32:	// STA a16, Store accumulator direct 
			uint16_t addr = opcode[1] << 8 | opcode[0];	
			state->memory[addr] = state->a;        
			break;

		case 0x3a:	// LDA a16, Load accumulator direct
			uint16_t addr = opcode[1] << 8 | opcode[0];	
			state->a = state->memory[addr];       
			break;

		/* INX, increment register pair */
		case 0x03:	// INX B 
			uint16_t result = (uint16_t)(getRegPair(state->b, state->c) + 1);
			state->b = (uint8_t) result;
			state->c = (uint8_t) (result >> 8);          
			break; 
		case 0x13:	// INX D
			uint16_t result = (uint16_t)(getRegPair(state->d, state->e) + 1);
			state->d = (uint8_t) result;
			state->e = (uint8_t) (result >> 8);               
			break; 
		case 0x23:	// INX H
			uint16_t result = (uint16_t)(getHLAddr(state) + 1);
			state->l = (uint8_t) result;
			state->h = (uint8_t) (result >> 8);                 
			break;
		case 0x33:	// INX SP
			uint16_t result = (uint16_t)(state->sp + 1);
			state->sp = result;           
			break;

		/* DCX, Decrement register pair */
		case 0x0b:	// DCX, B  
			uint16_t result = (uint16_t)(getRegPair(state->b, state->c) - 1);
			state->b = (uint8_t) result;
			state->c = (uint8_t) (result >> 8);
			break;
		case 0x1b:	// DCX, D  
			uint16_t result = (uint16_t)(getRegPair(state->d, state->e) - 1);
			state->d = (uint8_t) result;
			state->e = (uint8_t) (result >> 8);
			break;
		case 0x2b:	// DCX, H  
			uint16_t result = (uint16_t)(getHLAddr(state) - 1);
			state->l = (uint8_t) result;
			state->h = (uint8_t) (result >> 8);
			break;
		case 0x3b:	// DCX, SP  
			state->sp--;
			break;

		/* INR, Increment Register or Memory */
		case 0x04:	incRegOrMem(state, &state->b, 1);						break;	// INR B 
		case 0x14:	incRegOrMem(state, &state->d, 1);						break;	// INR D
		case 0x24:	incRegOrMem(state, &state->h, 1);						break; 	// INR H
		case 0x34:	incRegOrMem(state, &state->memory[getHLAddr(state)], 1); break;	// INR M
		case 0x0c:	incRegOrMem(state, &state->c, 1);						break;	// INR C
		case 0x1c:	incRegOrMem(state, &state->e, 1);						break;	// INR E
		case 0x2c:	incRegOrMem(state, &state->l, 1);						break;	// INR L
		case 0x3c:	incRegOrMem(state, &state->a, 1);						break;	// INR A
			
		/* DCR, Decrement Register or Memory */
		case 0x05:	decRegOrMem(state, &state->b, 1);						break;	// DCR B	
		case 0x15:	decRegOrMem(state, &state->d, 1); 						break;	// DCR D	
		case 0x25:	decRegOrMem(state, &state->h, 1);  						break;	// DCR H	
		case 0x35:	decRegOrMem(state, &state->memory[getHLAddr(state)], 1); break;	// DCR M	
		case 0x0d:	decRegOrMem(state, &state->c, 1);						break;	// DCR C	
		case 0x1d:	decRegOrMem(state, &state->e, 1);  						break;	// DCR E	
		case 0x2d:	decRegOrMem(state, &state->l, 1);  						break;	// DCR L	
		case 0x3d:	decRegOrMem(state, &state->a, 1); 						break;	// DCR A	
			
		/* MVI, Move Immediate Byte to Reg or Byte address*/ 
		case 0x06:	state->b = opcode[0];  									break;	// MVI B
		case 0x16:	state->c = opcode[0];  									break;	// MVI D
		case 0x26:	state->h = opcode[0]; 									break;	// MVI H
		case 0x36:	state->memory[getHLAddr(state)] = opcode[0];  			break;	// MVI M
		case 0x0e:	state->c = opcode[0]; 									break;	// MVI C
		case 0x1e:	state->e = opcode[0]; 									break;	// MVI E
		case 0x2e:	state->l = opcode[0]; 									break;	// MVI L
		case 0x3e:	state->a = opcode[0]; 									break;	// MVI A

		case 0x07:	// RLC, Rotate Accumulator Left
			state->cc.cy = (state->a & 0x80) ? 1 : 0;
			state->a = (state->a << 1) | state->cc.cy;
			break;
		case 0x0f:	// RRC, Rotate Accumulator Right
			state->cc.cy = (state->a & 0x01) ? 1 : 0;
			state->a = (state->a >> 1) | (state->cc.cy << 7);
			break;

		case 0x17:	// RAL, Rotate Accumulator Left Through Carry
			int highestOrderBit = (result & 0x80) ? 1 : 0;
			state->a = state->cc.cy ? (state->a << 1) | (state->cc.cy << 7) : (state->a << 1) & (state->cc.cy << 7);
			state->cc.cy = highestOrderBit;
			break;
		case 0x1f:	// RAR, Rotate Accumulator Right Through Carry
			int lowestOrderBit = (result & 0x01) ? 1 : 0;
			state->a = state->cc.cy ? (state->a >> 1) | (state->cc.cy << 7) : (state->a >> 1) & (state->cc.cy << 7);
			state->cc.cy = lowestOrderBit;
			break;	
		
		case 0x27:	// DAA, Decimal Adjust Accumulator
			 
			break;	
		case 0x2f: state->a = ~state->a; 									break;	// CMA, Complement Accumulator

		case 0x37: state->cc.cy = 1;										break;	// STC, Set Carry (to 1)
		case 0x3f: state->cc.cy = state->cc.cy ? 0 : 1; 					break;	// CMC, Complement Carry
		
		/* DAD, double add, rp + HL pair, 16 bit answer stored in HL pair */
		case 0x09:	// DAD B
			doubleAdd(state, getRegPair(state->b, state->c), getHLAddr(state));
			uint16_t result = getRegPair(state->b, state->c) + getHLAddr(state);
			state->l = (uint8_t) result;
			state->h = (uint8_t) (result >> 8);
			break;
		case 0x19:	// DAD D
			doubleAdd(state, getRegPair(state->d, state->e), getHLAddr(state));
			uint16_t result = getRegPair(state->d, state->e) + getHLAddr(state);
			state->l = (uint8_t) result;
			state->h = (uint8_t) (result >> 8);
			break;
		case 0x29:	// DAD H
			doubleAdd(state, getHLAddr(state), getHLAddr(state));
			uint16_t result = getHLAddr(state) << 1;
			state->l = (uint8_t) result;
			state->h = (uint8_t) (result >> 8);
			break;
		case 0x39:	// DAD SP
			doubleAdd(state, state->sp, getHLAddr(state));
			uint16_t result = state->sp + getHLAddr(state);
			state->l = (uint8_t) result;
			state->h = (uint8_t) (result >> 8);
			break;

		/* MOV */ 
		case 0x40:															break;
		case 0x41:	moveRegs(&state->b, &state->c); 						break;
		case 0x42:	moveRegs(&state->b, &state->d); 						break;
		case 0x43:	moveRegs(&state->b, &state->e); 						break;
		case 0x44:	moveRegs(&state->b, &state->h); 						break;
		case 0x45:	moveRegs(&state->b, &state->l); 						break;
		case 0x46:	moveRegs(&state->b, &state->memory[getHLAddr(state)]); 	break;
		case 0x47:	moveRegs(&state->b, &state->a); 						break;
		case 0x48:	moveRegs(&state->c, &state->b); 						break;
		case 0x49:															break;
		case 0x4a:	moveRegs(&state->c, &state->d); 						break;
		case 0x4b:	moveRegs(&state->c, &state->e); 						break;
		case 0x4c:	moveRegs(&state->c, &state->h); 						break;
		case 0x4d:	moveRegs(&state->c, &state->l); 						break;
		case 0x4e:	moveRegs(&state->c, &state->memory[getHLAddr(state)]); 	break;
		case 0x4f:	moveRegs(&state->c, &state->a); 						break;
		case 0x50:	moveRegs(&state->d, &state->b); 						break;
		case 0x51:	moveRegs(&state->d, &state->c); 						break;
		case 0x52:															break;
		case 0x53:	moveRegs(&state->d, &state->e); 						break;
		case 0x54:	moveRegs(&state->d, &state->h); 						break;
		case 0x55:	moveRegs(&state->d, &state->l); 						break;
		case 0x56:	moveRegs(&state->d, &state->memory[getHLAddr(state)]); 	break;
		case 0x57:	moveRegs(&state->d, &state->a); 						break;
		case 0x58:	moveRegs(&state->e, &state->b); 						break;
		case 0x59:	moveRegs(&state->e, &state->c); 						break;
		case 0x5a:	moveRegs(&state->e, &state->d); 						break;
		case 0x5b:															break;
		case 0x5c:	moveRegs(&state->e, &state->h); 						break;
		case 0x5d:	moveRegs(&state->e, &state->l);							break;
		case 0x5e:	moveRegs(&state->e, &state->memory[getHLAddr(state)]); 	break;
		case 0x5f:	moveRegs(&state->e, &state->a); 						break;
		case 0x60:	moveRegs(&state->h, &state->b); 						break;
		case 0x61:	moveRegs(&state->h, &state->c); 						break;
		case 0x62:	moveRegs(&state->h, &state->d); 						break;
		case 0x63:	moveRegs(&state->h, &state->e); 						break;
		case 0x64:															break;
		case 0x65:	moveRegs(&state->h, &state->l); 						break;
		case 0x66:	moveRegs(&state->h, &state->memory[getHLAddr(state)]); 	break;
		case 0x67:	moveRegs(&state->h, &state->a); 						break;
		case 0x68:	moveRegs(&state->l, &state->b); 						break;
		case 0x69:	moveRegs(&state->l, &state->c); 						break;
		case 0x6a:	moveRegs(&state->l, &state->d); 						break;
		case 0x6b:	moveRegs(&state->l, &state->e); 						break;
		case 0x6c:	moveRegs(&state->l, &state->h); 						break;
		case 0x6d:															break;
		case 0x6e:	moveRegs(&state->l, &state->memory[getHLAddr(state)]); 	break;
		case 0x6f:	moveRegs(&state->l, &state->a); 						break;
		case 0x78:	moveRegs(&state->a, &state->b); 						break;
		case 0x79:	moveRegs(&state->a, &state->c); 						break;
		case 0x7a:	moveRegs(&state->a, &state->d); 						break;
		case 0x7b:	moveRegs(&state->a, &state->e); 						break;
		case 0x7c:	moveRegs(&state->a, &state->h); 						break;
		case 0x7d:	moveRegs(&state->a, &state->l); 						break;
		case 0x7e:	moveRegs(&state->a, &state->memory[getHLAddr(state)]); 	break;
		case 0x7f:															break;
		case 0x70:	moveRegs(&state->memory[getHLAddr(state)], &state->b); 	break;
		case 0x71:	moveRegs(&state->memory[getHLAddr(state)], &state->c); 	break;
		case 0x72:	moveRegs(&state->memory[getHLAddr(state)], &state->d); 	break;
		case 0x73:	moveRegs(&state->memory[getHLAddr(state)], &state->e); 	break;
		case 0x74:	moveRegs(&state->memory[getHLAddr(state)], &state->h); 	break;
		case 0x75:	moveRegs(&state->memory[getHLAddr(state)], &state->l); 	break;
		case 0x77:	moveRegs(&state->memory[getHLAddr(state)], &state->a); 	break;

		case 0x76:	printf("HLT");  break;

		case 0x80:	ADD(state, &state->a, state->b); 						break;	// ADD B
		case 0x81:	ADD(state, &state->a, state->c);						break;	// ADD C
		case 0x82:	ADD(state, &state->a, state->d);						break;	// ADD D
		case 0x83:	ADD(state, &state->a, state->e);						break;	// ADD E
		case 0x84:	ADD(state, &state->a, state->h);						break;	// ADD H
		case 0x85:	ADD(state, &state->a, state->l);						break;	// ADD L
		case 0x86:	ADD(state, &state->a, state->memory[getHLAddr(state)]); break;	// ADD M
		case 0x87:	ADD(state, &state->a, state->a);						break;	// ADD A
			
		case 0x88:	ADC(state, &state->a, state->b); 						break;	// ADC B  
		case 0x89:	ADC(state, &state->a, state->c); 						break;	// ADC C  
		case 0x8a:	ADC(state, &state->a, state->c);						break;	// ADC D  
		case 0x8b:	ADC(state, &state->a, state->e);						break;	// ADC E  
		case 0x8c:	ADC(state, &state->a, state->h);						break;	// ADC H  
		case 0x8d:	ADC(state, &state->a, state->l);						break;	// ADC L  
		case 0x8e:	ADC(state, &state->a, state->memory[getHLAddr(state)]);	break;	// ADC M  
		case 0x8f:	ADC(state, &state->a, state->a);						break;	// ADC A  
			
		case 0x90:	SUB(state, &state->a, state->b);  						break;	// SUB B
		case 0x91:	SUB(state, &state->a, state->c);  						break;	// SUB C
		case 0x92:	SUB(state, &state->a, state->d);  						break;	// SUB D
		case 0x93:	SUB(state, &state->a, state->e);  						break;	// SUB E
		case 0x94:	SUB(state, &state->a, state->h);  						break;	// SUB H
		case 0x95:	SUB(state, &state->a, state->l);  						break;	// SUB L
		case 0x96:	SUB(state, &state->a, state->memory[getHLAddr(state)]);	break;	// SUB M
		case 0x97:	SUB(state, &state->a, state->a);  						break;	// SUB A
		
		case 0x98:	SBB(state, &state->a, state->b);						break;	// SBB B
		case 0x99:	SBB(state, &state->a, state->c);						break;	// SBB C
		case 0x9a:	SBB(state, &state->a, state->d);						break;	// SBB D
		case 0x9b:	SBB(state, &state->a, state->e);						break;	// SBB E
		case 0x9c:	SBB(state, &state->a, state->h);						break;	// SBB H	
		case 0x9d:	SBB(state, &state->a, state->l);						break;	// SBB L
		case 0x9e:	SBB(state, &state->a, state->memory[getHLAddr(state)]);	break;	// SBB M
		case 0x9f:	SBB(state, &state->a, state->a);						break;	// SBB A

		case 0xa0:	ANA(state, &state->a, state->b);						break;	// ANA B
		case 0xa1:	ANA(state, &state->a, state->c);  						break;	// ANA C
		case 0xa2:	ANA(state, &state->a, state->d);  						break;	// ANA D
		case 0xa3:	ANA(state, &state->a, state->e);  						break;	// ANA E
		case 0xa4:	ANA(state, &state->a, state->h);  						break;	// ANA H
		case 0xa5:	ANA(state, &state->a, state->l);  						break;	// ANA L
		case 0xa6:	ANA(state, &state->a, state->memory[getHLAddr(state)]); break;	// ANA M
		case 0xa7:	ANA(state, &state->a, state->a);  						break;	// ANA A

		case 0xa8:	XRA(state, &state->a, state->b);  						break;	// XRA B
		case 0xa9:	XRA(state, &state->a, state->c);  						break;	// XRA C
		case 0xaa:	XRA(state, &state->a, state->d);  						break;	// XRA D
		case 0xab:	XRA(state, &state->a, state->e);  						break;	// XRA E
		case 0xac:	XRA(state, &state->a, state->h);  						break;	// XRA H
		case 0xad:	XRA(state, &state->a, state->l);  						break;	// XRA L
		case 0xae:	XRA(state, &state->a, state->memory[getHLAddr(state)]);	break;	// XRA M
		case 0xaf:	XRA(state, &state->a, state->a);  						break;	// XRA A

		case 0xb0:	ORA(state, &state->a, state->b);  						break;	// ORA B
		case 0xb1:	ORA(state, &state->a, state->c);  						break;	// ORA C
		case 0xb2:	ORA(state, &state->a, state->d);  						break;	// ORA D
		case 0xb3:	ORA(state, &state->a, state->e);  						break;	// ORA E
		case 0xb4:	ORA(state, &state->a, state->h);  						break;	// ORA H
		case 0xb5:	ORA(state, &state->a, state->l);  						break;	// ORA L
		case 0xb6:	ORA(state, &state->a, state->memory[getHLAddr(state)]); break;	// ORA M
		case 0xb7:	ORA(state, &state->a, state->a);  						break;	// ORA A

		case 0xb8:	CMP(state, &state->a, state->b);  						break;	// CMP B
		case 0xb9:	CMP(state, &state->a, state->c);  						break;	// CMP C
		case 0xba:	CMP(state, &state->a, state->d);  						break;	// CMP D
		case 0xbb:	CMP(state, &state->a, state->e);  						break;	// CMP E
		case 0xbc:	CMP(state, &state->a, state->h);  						break;	// CMP H
		case 0xbd:	CMP(state, &state->a, state->l);  						break;	// CMP L
		case 0xbe:	CMP(state, &state->a, state->memory[getHLAddr(state)]);	break;	// CMP M
		case 0xbf:	CMP(state, &state->a, state->a);  						break;	// CMP A

		case 0xc6:	ADD(state, &state->a, opcode[0]);						break;	//ADI    
		case 0xce: 	ADC(state, &state->a, opcode[0]);						break;	//ACI   

		case 0xd6: 	SUB(state, &state->a, opcode[0]);						break;	//SUI    
		case 0xde: 	SBB(state, &state->a, opcode[0]);						break;	//SBI   

		case 0xe6:	ANA(state, &state->a, opcode[0]);						break;	//ANI    
		case 0xee:	XRA(state, &state->a, opcode[0]);						break;	//XRI    

		case 0xf6:	ORA(state, &state->a, opcode[0]);						break;	//ORI    
		case 0xfe:	CMP(state, &state->a, opcode[0]);						break;	//CPI    

		case 0xc0:	printf("RNZ");  break;
		case 0xd0:	printf("RNC");  break;
		case 0xe0:	printf("RPO");  break;
		case 0xf0:	printf("RP");  break;

		case 0xc1:	printf("POP    B");  break;
		case 0xd1:	printf("POP    D");  break;
		case 0xe1:	printf("POP    H");  break;
		case 0xf1:	printf("POP    PSW");  break;

		case 0xc2:	printf("JNZ    adr,#$%02x%02x", opcode[2], opcode[1]);  break;
		case 0xd2:	printf("JNC    adr,#$%02x%02x", opcode[2], opcode[1]);  break;
		case 0xe2:	printf("JPO    adr,#$%02x%02x", opcode[2], opcode[1]);  break;
		case 0xf2:	printf("JP     adr,#$%02x%02x", opcode[2], opcode[1]);  break;

		case 0xc3:
		case 0xcb:	printf("JMP    adr,#$%02x%02x", opcode[2], opcode[1]);  break;

		case 0xc4:	printf("CNZ    adr,#$%02x%02x", opcode[2], opcode[1]);  break; 
		case 0xd4:	printf("CNC    adr,#$%02x%02x", opcode[2], opcode[1]);  break;
		case 0xe4:	printf("CPO    adr,#$%02x%02x", opcode[2], opcode[1]);  break;
		case 0xf4:	printf("CP     adr,#$%02x%02x", opcode[2], opcode[1]);  break;

		case 0xc5:	printf("PUSH   B");  break;
		case 0xd5:	printf("PUSH   D");  break;
		case 0xe5:	printf("PUSH   H");  break;
		case 0xf5:	printf("PUSH   PSW");  break;

		case 0xc7:	printf("RST");  break;
		case 0xcf:	printf("RST");  break;
		case 0xd7:	printf("RST,#$%02x", opcode[1]);  break;
		case 0xdf:	printf("RST");  break;
		case 0xef:	printf("RST");  break;
		case 0xe7:	printf("RST");  break;
		case 0xf7:	printf("RST");  break;
		case 0xff:	printf("RST");  break;

		case 0xc8:	printf("RZ");  break;
		case 0xd8:	printf("RC");  break;
		case 0xe8:	printf("RPE");  break;
		case 0xf8:	printf("RM");  break;

		case 0xc9:	
		case 0xd9:	printf("RET");  break;

		case 0xca:	printf("JZ     adr,#$%02x%02x", opcode[2], opcode[1]);  break; 
		case 0xda:	printf("JC     adr,#$%02x%02x", opcode[2], opcode[1]);  break;
		case 0xea:	printf("JPE    adr,#$%02x%02x", opcode[2], opcode[1]);  break;
		case 0xfa:	printf("JM     adr,#$%02x%02x", opcode[2], opcode[1]);  break;
		
		case 0xcd:	
		case 0xdd:	
		case 0xed:	
		case 0xfd:	printf("CALL   adr,#$%02x%02x", opcode[2], opcode[1]);  break; 

		case 0xcc:	printf("CZ     adr,#$%02x%02x", opcode[2], opcode[1]);  break; 
		case 0xdc:	printf("CC     adr,#$%02x%02x", opcode[2], opcode[1]);  break;
		case 0xec:	printf("CPE    adr,#$%02x%02x", opcode[2], opcode[1]);  break;
		case 0xfc:	printf("CM     adr,#$%02x%02x", opcode[2], opcode[1]);  break;
		
		case 0xd3:	printf("OUT    D8,#$%02x", opcode[1]);  break;
		case 0xdb:	printf("IN     D8,#$%02x", opcode[1]);  break;
		
		case 0xe3:	printf("XTHL");  break;
		case 0xeb:	printf("XCHG");  break;
		
		case 0xe9:	printf("PCHL");  break;
		case 0xf9:	printf("SPHL");  break;
	
		case 0xf3:	printf("DI");  break;
		case 0xfb:	printf("EI");  break;
	}

	return state->pc += opcodeSize[(column * 16) + row];
}
