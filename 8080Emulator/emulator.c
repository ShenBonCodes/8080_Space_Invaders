#include <stdio.h>
#include <stdlib.h>
#include "emulator.h"

void Emulate8080(State8080* state);

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

int parityCheck(uint8_t val1, uint8_t val2)
{
	uint8_t result = val1 + val2;
	int oneCount = 0, result = 0;
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

void update_s_flag(State8080* state, uint8_t val1, uint8_t val2)
{	// set to 1 when bit 7 (the most significant bit or MSB) of the math instruction is set
	uint8_t result = val1 + val2;

	if ((result & 0x80)) // == 1
		state->cc.s = 1;
	else
		state->cc.s = 0;
}

void update_z_flag(State8080* state, uint8_t val1, uint8_t val2) 
{	// set to 1 when the result is equal to zero
	uint8_t result = val1 + val2;

	if (result == 0)
		state->cc.z = 1;
	else
		state->cc.z = 0;
}

void update_ac_flag_add(State8080* state, uint8_t val1, uint8_t val2)
{
	val1 = val1 & ~(0x10); // clears 5th bit
	uint8_t result = val1 + val2;

	state->cc.ac = result & 0x10 ? 1 : 0;
}

void update_ac_flag_sub(State8080* state, uint8_t val1, uint8_t val2)
{
	val1 = val1 & ~(0x10); // clears 5th bit
	val2 = (uint8_t)~val2 + 1; // 2's complement
	uint8_t result = val1 + val2;

	state->cc.ac = result & 0x10 ? 1 : 0;
}

void update_p_flag(State8080* state, uint8_t val1, uint8_t val2)
{	// set when the answer has even parity, clear when odd parity
	state->cc.p = parityCheck(val1, val2);
}

void update_c_flag(State8080* state, uint8_t val1, uint8_t val2)
{	// set to 1 when the instruction resulted in a carry out or borrow into the high order bit
	uint16_t result = (uint16_t)val1 + (uint16_t)val2;

	if (result > 0xff)    
		state->cc.cy = 1;    
	else    
		state->cc.cy = 0;    
}

void movRegs(uint8_t* dest, uint8_t* src)
{
	*dest = *src;
}

void incRegOrMem(State8080* state, uint8_t val1, uint8_t val2)
{	
	update_s_flag(state, val1, val2);
	update_z_flag(state, val1, val2);
	update_ac_flag_add(state, val1, val2);
	update_p_flag(state, val1, val2);
}

void decRegOrMem(State8080* state, uint8_t val1, uint8_t val2)
{	
	update_s_flag(state, val1, val2);
	update_z_flag(state, val1, val2);
	update_ac_flag_sub(state, val1, -val2);
	update_p_flag(state, val1, val2);
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
		case 0x01:	// LXI B
			state->c = opcode[1];   // little-endian, second byte is least significant 
			state->b = opcode[2];    
			state->pc += 2;     
			break;    
		case 0x11:	// LXI D, d16
			state->e = opcode[1];    
			state->d = opcode[2];    
			state->pc += 2;                   
			break;
		case 0x21:	// LXI H, d16
			state->l = opcode[1];    
			state->h = opcode[2];    
			state->pc += 2;                  
			break;
		case 0x31:	// LXI SP, d16
			// shift left 8 bits, then use OR operator to put add LSB to right hand side
			// because 0 | x = x (x is 0 or 1), so right 8 bits will just be LSB (least significant bits)
			uint16_t addr = opcode[2] << 8 | opcode[1];
			state->sp = addr;  
			state->pc += 2;                  
			break;
		
		/* STAX, store accumulator */
		case 0x02:	// STAX B
			state->memory[getRegPair(state->b, state->c)] = state->a;
			break;
		case 0x12:	// STAX D
			state->memory[getRegPair(state->d, state->e)] = state->a;
			break;

		/* LDAX, load accumulator */
		case 0x0a:	// LDAX B, load accumulator
			state->a = state->memory[getRegPair(state->b, state->c)];
			break; 
		case 0x1a:	// LDAX C
			state->a = state->memory[getRegPair(state->d, state->e)];
			break; 
		
		case 0x22:	// SHLD, Store H and L to memory
			uint16_t addr = opcode[1] << 8 | opcode[0];	
			state->memory[addr] = state->l;
			state->memory[(uint16_t)(addr + 1)] = state->h;     
			state->pc += 2;  
			break; 

		case 0x2a:	// LHLD, Load H and L from
			uint16_t addr = opcode[1] << 8 | opcode[0];	
			state->l = state->memory[addr]; 
			state->h = state->memory[(uint16_t)(addr + 1)];
			state->pc += 2;
			break;

		case 0x32:	// STA a16, Store accumulator direct 
			uint16_t addr = opcode[1] << 8 | opcode[0];	
			state->memory[addr] = state->a;
			state->pc += 2;               
			break;

		case 0x3a:	// LDA a16, Load accumulator direct
			uint16_t addr = opcode[1] << 8 | opcode[0];	
			state->a = state->memory[addr];
			state->pc += 2;               
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
		case 0x04:	// INR B 
			incRegOrMem(state, state->b, 1);
			state->b++;
			break;
		case 0x14:	// INR D
			incRegOrMem(state, state->d, 1);
			state->d++;
			break;
		case 0x24:	// INR H
			incRegOrMem(state, state->h, 1);
			state->h++;
			break;
		case 0x34:	// INR M
			uint16_t addr = getHLAddr(state);
			incRegOrMem(state, state->memory[addr], 1);
			state->memory[addr]++;
			break;
		case 0x0c:	// INR C
			incRegOrMem(state, state->c, 1);
			state->c++;
			break;
		case 0x1c:	// INR E
			incRegOrMem(state, state->e, 1);
			state->e++;
			break;
		case 0x2c:	// INR L
			incRegOrMem(state, state->l, 1);
			state->l++;
			break;
		case 0x3c:	// INR A
			incRegOrMem(state, state->a, 1);
			state->a++;
			break;

		/* DCR, Decrement Register or Memory */
		case 0x05:	// DCR B	
			decRegOrMem(state, state->b, -1);  
			state->b--;
			break;
		case 0x15:	// DCR D	
			decRegOrMem(state, state->d, -1);  
			state->d--;
			break;
		case 0x25:	// DCR H	
			decRegOrMem(state, state->h, -1);  
			state->h--;
			break;
		case 0x35:	// DCR M	
			uint16_t addr = getHLAddr(state);
			decRegOrMem(state, state->memory[addr], -1);  
			state->memory[addr]--;
			break;
		case 0x0d:	// DCR C	
			decRegOrMem(state, state->c, -1);  
			state->c--;
			break;
		case 0x1d:	// DCR E	
			decRegOrMem(state, state->e, -1); 
			state->e--; 
			break;
		case 0x2d:	// DCR L	
			decRegOrMem(state, state->l, -1);  
			state->l--;
			break;
		case 0x3d:	// DCR A	
			decRegOrMem(state, state->a, -1);  
			state->a--;
			break;
		
		/* MVI, Move Immediate Byte to Reg or Byte address*/ 
		case 0x06:	state->b = opcode[0]; state->pc += 1; break; 	// MVI B
		case 0x16:	state->c = opcode[0]; state->pc += 1; break;	// MVI D
		case 0x26:	state->h = opcode[0]; state->pc += 1; break;	// MVI H
		case 0x36:	state->memory[getHLAddr(state)] = opcode[0]; state->pc += 1; break;	// MVI M
		case 0x0e:	state->c = opcode[0]; state->pc += 1; break;	// MVI C
		case 0x1e:	state->e = opcode[0]; state->pc += 1; break;	// MVI E
		case 0x2e:	state->l = opcode[0]; state->pc += 1; break;	// MVI L
		case 0x3e:	state->a = opcode[0]; state->pc += 1; break;	// MVI A

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
		case 0x2f:	// CMA, Complement Accumulator
			state->a = ~state->a;
			break;

		case 0x37:	// STC, Set Carry (to 1)
			state->cc.cy = 1;
			break;
		case 0x3f:	// CMC, Complement Carry
			state->cc.cy = state->cc.cy ? 0 : 1;
			break;
		
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
		case 0x40:	break;
		case 0x41:	moveRegs(&state->b, &state->c); break;
		case 0x42:	moveRegs(&state->b, &state->d); break;
		case 0x43:	moveRegs(&state->b, &state->e); break;
		case 0x44:	moveRegs(&state->b, &state->h); break;
		case 0x45:	moveRegs(&state->b, &state->l); break;
		case 0x46:	moveRegs(&state->b, &state->memory[getHLAddr(state)]); break;
		case 0x47:	moveRegs(&state->b, &state->a); break;
		case 0x48:	moveRegs(&state->c, &state->b); break;
		case 0x49:	break;
		case 0x4a:	moveRegs(&state->c, &state->d); break;
		case 0x4b:	moveRegs(&state->c, &state->e); break;
		case 0x4c:	moveRegs(&state->c, &state->h); break;
		case 0x4d:	moveRegs(&state->c, &state->l); break;
		case 0x4e:	moveRegs(&state->c, &state->memory[getHLAddr(state)]); break;
		case 0x4f:	moveRegs(&state->c, &state->a); break;
		case 0x50:	moveRegs(&state->d, &state->b); break;
		case 0x51:	moveRegs(&state->d, &state->c); break;
		case 0x52:	break;
		case 0x53:	moveRegs(&state->d, &state->e); break;
		case 0x54:	moveRegs(&state->d, &state->h); break;
		case 0x55:	moveRegs(&state->d, &state->l); break;
		case 0x56:	moveRegs(&state->d, &state->memory[getHLAddr(state)]); break;
		case 0x57:	moveRegs(&state->d, &state->a); break;
		case 0x58:	moveRegs(&state->e, &state->b); break;
		case 0x59:	moveRegs(&state->e, &state->c); break;
		case 0x5a:	moveRegs(&state->e, &state->d); break;
		case 0x5b:	break;
		case 0x5c:	moveRegs(&state->e, &state->h); break;
		case 0x5d:	moveRegs(&state->e, &state->l); break;
		case 0x5e:	moveRegs(&state->e, &state->memory[getHLAddr(state)]); break;
		case 0x5f:	moveRegs(&state->e, &state->a); break;
		case 0x60:	moveRegs(&state->h, &state->b); break;
		case 0x61:	moveRegs(&state->h, &state->c); break;
		case 0x62:	moveRegs(&state->h, &state->d); break;
		case 0x63:	moveRegs(&state->h, &state->e); break;
		case 0x64:	break;
		case 0x65:	moveRegs(&state->h, &state->l); break;
		case 0x66:	moveRegs(&state->h, &state->memory[getHLAddr(state)]); break;
		case 0x67:	moveRegs(&state->h, &state->a); break;
		case 0x68:	moveRegs(&state->l, &state->b); break;
		case 0x69:	moveRegs(&state->l, &state->c); break;
		case 0x6a:	moveRegs(&state->l, &state->d); break;
		case 0x6b:	moveRegs(&state->l, &state->e); break;
		case 0x6c:	moveRegs(&state->l, &state->h); break;
		case 0x6d:	break;
		case 0x6e:	moveRegs(&state->l, &state->memory[getHLAddr(state)]); break;
		case 0x6f:	moveRegs(&state->l, &state->a); break;
		case 0x78:	moveRegs(&state->a, &state->b); break;
		case 0x79:	moveRegs(&state->a, &state->c); break;
		case 0x7a:	moveRegs(&state->a, &state->d); break;
		case 0x7b:	moveRegs(&state->a, &state->e); break;
		case 0x7c:	moveRegs(&state->a, &state->h); break;
		case 0x7d:	moveRegs(&state->a, &state->l); break;
		case 0x7e:	moveRegs(&state->a, &state->memory[getHLAddr(state)]); break;
		case 0x7f:	break;
		case 0x70:	moveRegs(&state->memory[getHLAddr(state)], &state->b); break;
		case 0x71:	moveRegs(&state->memory[getHLAddr(state)], &state->c); break;
		case 0x72:	moveRegs(&state->memory[getHLAddr(state)], &state->d); break;
		case 0x73:	moveRegs(&state->memory[getHLAddr(state)], &state->e); break;
		case 0x74:	moveRegs(&state->memory[getHLAddr(state)], &state->h); break;
		case 0x75:	moveRegs(&state->memory[getHLAddr(state)], &state->l); break;
		case 0x77:	moveRegs(&state->memory[getHLAddr(state)], &state->a); break;
		/* END OF MOV */

		case 0x76:	printf("HLT");  break;

		case 0x80:	printf("ADD    B");  break;
		case 0x81:	printf("ADD    C");  break;
		case 0x82:	printf("ADD    D");  break;
		case 0x83:	printf("ADD    E");  break;
		case 0x84:	printf("ADD    H");  break;
		case 0x85:	printf("ADD    L");  break;
		case 0x86:	printf("ADD    M");  break;
		case 0x87:	printf("ADD    A");  break;
		case 0x88:	printf("ADC    B");  break;
		case 0x89:	printf("ADC    C");  break;
		case 0x8a:	printf("ADC    D");  break;
		case 0x8b:	printf("ADC    E");  break;
		case 0x8c:	printf("ADC    H");  break;
		case 0x8d:	printf("ADC    L");  break;
		case 0x8e:	printf("ADC    M");  break;
		case 0x8f:	printf("ADC    A");  break;
		case 0x90:	printf("SUB    B");  break;
		case 0x91:	printf("SUB    C");  break;
		case 0x92:	printf("SUB    D");  break;
		case 0x93:	printf("SUB    E");  break;
		case 0x94:	printf("SUB    H");  break;
		case 0x95:	printf("SUB    L");  break;
		case 0x96:	printf("SUB    M");  break;
		case 0x97:	printf("SUB    A");  break;
		case 0x98:	printf("SBB    B");  break;
		case 0x99:	printf("SBB    C");  break;
		case 0x9a:	printf("SBB    D");  break;
		case 0x9b:	printf("SBB    E");  break;
		case 0x9c:	printf("SBB    H");  break;
		case 0x9d:	printf("SBB    L");  break;
		case 0x9e:	printf("SBB    M");  break;
		case 0x9f:	printf("SBB    A");  break;
		case 0xa0:	printf("ANA    B");  break;
		case 0xa1:	printf("ANA    C");  break;
		case 0xa2:	printf("ANA    D");  break;
		case 0xa3:	printf("ANA    E");  break;
		case 0xa4:	printf("ANA    H");  break;
		case 0xa5:	printf("ANA    L");  break;
		case 0xa6:	printf("ANA    M");  break;
		case 0xa7:	printf("ANA    A");  break;
		case 0xa8:	printf("XRA    B");  break;
		case 0xa9:	printf("XRA    C");  break;
		case 0xaa:	printf("XRA    D");  break;
		case 0xab:	printf("XRA    E");  break;
		case 0xac:	printf("XRA    H");  break;
		case 0xad:	printf("XRA    L");  break;
		case 0xae:	printf("XRA    M");  break;
		case 0xaf:	printf("XRA    A");  break;
		case 0xb0:	printf("ORA    B");  break;
		case 0xb1:	printf("ORA    C");  break;
		case 0xb2:	printf("ORA    D");  break;
		case 0xb3:	printf("ORA    E");  break;
		case 0xb4:	printf("ORA    H");  break;
		case 0xb5:	printf("ORA    L");  break;
		case 0xb6:	printf("ORA    M");  break;
		case 0xb7:	printf("ORA    A");  break;
		case 0xb8:	printf("CMP    B");  break;
		case 0xb9:	printf("CMP    C");  break;
		case 0xba:	printf("CMP    D");  break;
		case 0xbb:	printf("CMP    E");  break;
		case 0xbc:	printf("CMP    H");  break;
		case 0xbd:	printf("CMP    L");  break;
		case 0xbe:	printf("CMP    M");  break;
		case 0xbf:	printf("CMP    A");  break;
		case 0xc0:	printf("RNZ");  break;
		case 0xc1:	printf("POP    B");  break;
		case 0xc2:	printf("JNZ    adr,#$%02x%02x", opcode[2], opcode[1]); state->pc += 2; break;
		case 0xc3:	printf("JMP    adr,#$%02x%02x", opcode[2], opcode[1]); state->pc += 2; break;
		case 0xc4:	printf("CNZ    adr,#$%02x%02x", opcode[2], opcode[1]); state->pc += 2; break; 
		case 0xc5:	printf("PUSH   B");  break;
		case 0xc6:	printf("ADI    D8,#$%02x", opcode[1]); state->pc += 1; break;
		case 0xc7:	printf("RST");  break;
		case 0xc8:	printf("RZ");  break;
		case 0xc9:	printf("RET");  break;
		case 0xca:	printf("JZ     adr,#$%02x%02x", opcode[2], opcode[1]); state->pc += 2; break; 
		case 0xcb:	printf("JMP    adr,#$%02x%02x", opcode[2], opcode[1]); state->pc += 2; break;
		case 0xcc:	printf("CZ     adr,#$%02x%02x", opcode[2], opcode[1]); state->pc += 2; break; 
		case 0xcd:	printf("CALL   adr,#$%02x%02x", opcode[2], opcode[1]); state->pc += 2; break; 
		case 0xce:	printf("ACI    D8,#$%02x", opcode[1]); state->pc += 1; break;
		case 0xcf:	printf("RST");  break;
		case 0xd0:	printf("RNC");  break;
		case 0xd1:	printf("POP    D");  break;
		case 0xd2:	printf("JNC    adr,#$%02x%02x", opcode[2], opcode[1]); state->pc += 2; break;
		case 0xd3:	printf("OUT    D8,#$%02x", opcode[1]); state->pc += 1; break;
		case 0xd4:	printf("CNC    adr,#$%02x%02x", opcode[2], opcode[1]); state->pc += 2; break;
		case 0xd5:	printf("PUSH   D");  break;
		case 0xd6:	printf("SUI    D8,#$%02x", opcode[1]); state->pc += 1; break;
		case 0xd7:	printf("RST,#$%02x", opcode[1]); state->pc += 1; break;
		case 0xd8:	printf("RC");  break;
		case 0xd9:	printf("RET");  break;
		case 0xda:	printf("JC     adr,#$%02x%02x", opcode[2], opcode[1]); state->pc += 2; break;
		case 0xdb:	printf("IN     D8,#$%02x", opcode[1]); state->pc += 1; break;
		case 0xdc:	printf("CC     adr,#$%02x%02x", opcode[2], opcode[1]); state->pc += 2; break;
		case 0xdd:	printf("CALL   adr,#$%02x%02x", opcode[2], opcode[1]); state->pc += 2; break; 
		case 0xde:	printf("SBI    D8,#$%02x", opcode[1]); state->pc += 1; break;
		case 0xdf:	printf("RST");  break;
		case 0xe0:	printf("RPO");  break;
		case 0xe1:	printf("POP    H");  break;
		case 0xe2:	printf("JPO    adr,#$%02x%02x", opcode[2], opcode[1]); state->pc += 2; break;
		case 0xe3:	printf("XTHL");  break;
		case 0xe4:	printf("CPO    adr,#$%02x%02x", opcode[2], opcode[1]); state->pc += 2; break;
		case 0xe5:	printf("PUSH   H");  break;
		case 0xe6:	printf("ANI    D8,#$%02x", opcode[1]); state->pc += 1; break;
		case 0xe7:	printf("RST");  break;
		case 0xe8:	printf("RPE");  break;
		case 0xe9:	printf("PCHL");  break;
		case 0xea:	printf("JPE    adr,#$%02x%02x", opcode[2], opcode[1]); state->pc += 2; break;
		case 0xeb:	printf("XCHG");  break;
		case 0xec:	printf("CPE    adr,#$%02x%02x", opcode[2], opcode[1]); state->pc += 2; break;
		case 0xed:	printf("CALL   adr,#$%02x%02x", opcode[2], opcode[1]); state->pc += 2; break; 
		case 0xee:	printf("XRI    D8,#$%02x", opcode[1]); state->pc += 1; break;
		case 0xef:	printf("RST");  break;
		case 0xf0:	printf("RP");  break;
		case 0xf1:	printf("POP    PSW");  break;
		case 0xf2:	printf("JP     adr,#$%02x%02x", opcode[2], opcode[1]); state->pc += 2; break;
		case 0xf3:	printf("DI");  break;
		case 0xf4:	printf("CP     adr,#$%02x%02x", opcode[2], opcode[1]); state->pc += 2; break;
		case 0xf5:	printf("PUSH   PSW");  break;
		case 0xf6:	printf("ORI    D8,#$%02x", opcode[1]); state->pc += 1; break;
		case 0xf7:	printf("RST");  break;
		case 0xf8:	printf("RM");  break;
		case 0xf9:	printf("SPHL");  break;
		case 0xfa:	printf("JM     adr,#$%02x%02x", opcode[2], opcode[1]); state->pc += 2; break;
		case 0xfb:	printf("EI");  break;
		case 0xfc:	printf("CM     adr,#$%02x%02x", opcode[2], opcode[1]); state->pc += 2; break;
		case 0xfd:	printf("CALL   adr,#$%02x%02x", opcode[2], opcode[1]); state->pc += 2; break; 
		case 0xfe:	printf("CPI    D8,#$%02x", opcode[1]); state->pc += 1; break;
		case 0xff:	printf("RST");  break;
	}
	state->pc++;
}
