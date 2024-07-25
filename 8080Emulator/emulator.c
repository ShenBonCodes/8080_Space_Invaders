#include <stdio.h>
#include <stdlib.h>
#include "emulator.h"
#include <cstdint>

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

void movRegs(uint8_t* dest, uint8_t* src)
{
	*dest = *src;
}

void movRegsToMem(uint8_t* src, uint8_t* mem, uint16_t addr)
{
	mem[addr] = *src;
}

uint16_t getRPAddr(uint8_t* msb, uint8_t* lsb) // returns address created RP concatonation
{
	uint16_t addr = *msb << 8 | *lsb;
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

		case 0x01:	// LXI B, d16, load 16-bit immediate data into rp (B, D, H, SP)
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

		case 0x04:	printf("INR    B"); opbytes = 1; break;
		case 0x05:	printf("DCR    B"); opbytes = 1; break;
		case 0x06:	printf("MVI    B,#$%02x", codeLine[1]); opbytes = 2; break;
		case 0x07:	printf("RLC"); opbytes = 1; break;	
		
		case 0x09:	printf("DAD    B"); opbytes = 1; break;
		case 0x0a:	printf("LDAX   B"); opbytes = 1; break;
		case 0x0b:	printf("DCX    B"); opbytes = 1; break;
		case 0x0c:	printf("INR    C"); opbytes = 1; break;
		case 0x0d:	printf("DCR    C"); opbytes = 1; break;
		case 0x0e:	printf("MVI    C,#$%02x", codeLine[1]); opbytes = 2; break;
		case 0x0f:	printf("RRC"); opbytes = 1; break;	
		
		/* If register B contains 3FH and register C contains 16H, the instruction: 
									STAX B
		will store the contents of the accumulator at memory location 3F16H. */
		case 0x02:	// STAX B
			uint16_t addr = state->b << 8 | state->c;
			state->memory[addr] = state->a;
			break;
		case 0x12:	// STAX D
			uint16_t addr = state->d << 8 | state->e;	
			state->memory[addr] = state->a;            
			break;
		
		case 0x14:	printf("INR    D"); opbytes = 1; break;
		case 0x15:	printf("DCR    D"); opbytes = 1; break;
		case 0x16:	printf("MVI    C,#$%02x", codeLine[1]); opbytes = 2; break;
		case 0x17:	printf("RAL"); opbytes = 1; break;	
		
		case 0x19:	printf("DAD    D"); opbytes = 1; break;
		case 0x1a:	printf("LDAX   D"); opbytes = 1; break;
		case 0x1b:	printf("DCX    D"); opbytes = 1; break;
		case 0x1c:	printf("INR    E"); opbytes = 1; break;
		case 0x1d:	printf("DCR    E"); opbytes = 1; break;
		case 0x1e:	printf("MVI    E,#$%02x", codeLine[1]); opbytes = 2; break;
		case 0x1f:	printf("RAR"); opbytes = 1; break;	
		
		case 0x22:	// SHLD, Store Hand L DirectLabel
			uint16_t addr = opcode[1] << 8 | opcode[0];	
			state->memory[addr] = state->l;
			state->memory[(uint16_t)(addr + 1)] = state->h;     
			break; 

		case 0x32:	// STA a16  
			uint16_t addr = opcode[1] << 8 | opcode[0];	
			state->memory[addr] = state->l;
			state->memory[(uint16_t)(addr + 1)] = state->h;               
			break;

		case 0x03:	// INX B, increment register pair
			state->b++;
			state->c++;                  
			break; 
		case 0x13:	// INX D
			state->d++;
			state->e++;                  
			break; 
		case 0x23:	// INX H
			state->h++;
			state->l++;                  
			break;
		case 0x33:	// INX SP
			state->sp++;                 
			break;

		case 0x24:	printf("INR    H"); opbytes = 1; break;
		case 0x25:	printf("DCR    H"); opbytes = 1; break;
		case 0x26:	printf("MVI    H,#$%02x", codeLine[1]); opbytes = 2; break;
		case 0x27:	printf("DAA"); opbytes = 1; break;	
		
		case 0x29:	printf("DAD    H"); opbytes = 1; break;
		case 0x2a:	printf("LHLD   adr,#$%02x%02x", codeLine[2], codeLine[1]); opbytes = 3; break; 
		case 0x2b:	printf("DCX    H"); opbytes = 1; break;
		case 0x2c:	printf("INR    L"); opbytes = 1; break;
		case 0x2d:	printf("DCR    L"); opbytes = 1; break;
		case 0x2e:	printf("MVI    L,#$%02x", codeLine[1]); opbytes = 2; break;
		case 0x2f:	printf("CMA"); opbytes = 1; break;
		
		case 0x34:	printf("INR    M");  opbytes = 1; break;
		case 0x35:	printf("DCR    M");  opbytes = 1; break;
		case 0x36:	printf("MVI    M,#$%02x", codeLine[1]); opbytes = 2; break;
		case 0x37:	printf("STC"); opbytes = 1; break;
		
		case 0x39:	printf("DAD    SP"); opbytes = 1; break;
		case 0x3a:	printf("LDA    adr,#$%02x%02x", codeLine[2], codeLine[1]); opbytes = 3; break;
		case 0x3b:	printf("DCX    SP"); opbytes = 1; break;
		case 0x3c:	printf("INR    A"); opbytes = 1; break;
		case 0x3d:	printf("DCR    A"); opbytes = 1; break;
		case 0x3e:	printf("MVI    A,#$%02x", codeLine[1]); opbytes = 2; break;
		case 0x3f:	printf("CMC"); opbytes = 1; break;

		case 0x40:	moveRegs(state->b, state->b) break;
		case 0x41:	moveRegs(state->b, state->c) break;
		case 0x42:	moveRegs(state->b, state->d) break;
		case 0x43:	moveRegs(state->b, state->e) break;
		case 0x44:	moveRegs(state->b, state->h) break;
		case 0x45:	moveRegs(state->b, state->l) break;
		case 0x46:	moveRegs(state->b, state->memory[(uint16_t)(state->h << 8 | state->l])) break;
		case 0x47:	moveRegs(state->b, state->a) break;
		case 0x48:	moveRegs(state->c, state->b) break;
		case 0x49:	moveRegs(state->c, state->c) break;
		case 0x4a:	moveRegs(state->c, state->d) break;
		case 0x4b:	moveRegs(state->c, state->e) break;
		case 0x4c:	moveRegs(state->c, state->h) break;
		case 0x4d:	moveRegs(state->c, state->l) break;
		case 0x4e:	moveRegs(state->c,M) break;
		case 0x4f:	moveRegs(state->c, state->a) break;
		case 0x50:	moveRegs(state->d, state->b) break;
		case 0x51:	moveRegs(state->d, state->c) break;
		case 0x52:	moveRegs(state->d, state->d) break;
		case 0x53:	moveRegs(state->d, state->e) break;
		case 0x54:	moveRegs(state->d, state->h) break;
		case 0x55:	moveRegs(state->d, state->l) break;
		case 0x56:	moveRegs(state->d,M) break;
		case 0x57:	moveRegs(state->d, state->a) break;
		case 0x58:	moveRegs(state->e, state->b) break;
		case 0x59:	moveRegs(state->e, state->c) break;
		case 0x5a:	moveRegs(state->e, state->d) break;
		case 0x5b:	moveRegs(state->e, state->e) break;
		case 0x5c:	moveRegs(state->e, state->h) break;
		case 0x5d:	moveRegs(state->e, state->l) break;
		case 0x5e:	moveRegs(state->e,M) break;
		case 0x5f:	moveRegs(state->e, state->a) break;
		case 0x60:	moveRegs(state->h, state->b) break;
		case 0x61:	moveRegs(state->h, state->c) break;
		case 0x62:	moveRegs(state->h, state->d) break;
		case 0x63:	moveRegs(state->h, state->e) break;
		case 0x64:	moveRegs(state->h, state->h) break;
		case 0x65:	moveRegs(state->h, state->l) break;
		case 0x66:	moveRegs(state->h,M) break;
		case 0x67:	moveRegs(state->h, state->a) break;
		case 0x68:	moveRegs(state->l, state->b) break;
		case 0x69:	moveRegs(state->l, state->c) break;
		case 0x6a:	moveRegs(state->l, state->d) break;
		case 0x6b:	moveRegs(state->l, state->e) break;
		case 0x6c:	moveRegs(state->l, state->h) break;
		case 0x6d:	moveRegs(state->l, state->l) break;
		case 0x6e:	moveRegs(state->l,M) break;
		case 0x6f:	moveRegs(state->l, state->a) break;
		case 0x78:	moveRegs(state->a, state->b) break;
		case 0x79:	moveRegs(state->a, state->c) break;
		case 0x7a:	moveRegs(state->a, state->d) break;
		case 0x7b:	moveRegs(state->a, state->e) break;
		case 0x7c:	moveRegs(state->a, state->h) break;
		case 0x7d:	moveRegs(state->a, state->l) break;
		case 0x7e:	moveRegs(state->a,M) break;
		case 0x7f:	moveRegs(state->a, state->a) break;

		case 0x70:	M,B break;
		case 0x71:	M,C break;
		case 0x72:	M,D break;
		case 0x73:	M,E break;
		case 0x74:	M,H break;
		case 0x75:	M,L break;
		case 0x77:	M,A break;

		

		case 0x76:	printf("HLT"); opbytes = 1; break;

		case 0x80:	printf("ADD    B"); opbytes = 1; break;
		case 0x81:	printf("ADD    C"); opbytes = 1; break;
		case 0x82:	printf("ADD    D"); opbytes = 1; break;
		case 0x83:	printf("ADD    E"); opbytes = 1; break;
		case 0x84:	printf("ADD    H"); opbytes = 1; break;
		case 0x85:	printf("ADD    L"); opbytes = 1; break;
		case 0x86:	printf("ADD    M"); opbytes = 1; break;
		case 0x87:	printf("ADD    A"); opbytes = 1; break;
		case 0x88:	printf("ADC    B"); opbytes = 1; break;
		case 0x89:	printf("ADC    C"); opbytes = 1; break;
		case 0x8a:	printf("ADC    D"); opbytes = 1; break;
		case 0x8b:	printf("ADC    E"); opbytes = 1; break;
		case 0x8c:	printf("ADC    H"); opbytes = 1; break;
		case 0x8d:	printf("ADC    L"); opbytes = 1; break;
		case 0x8e:	printf("ADC    M"); opbytes = 1; break;
		case 0x8f:	printf("ADC    A"); opbytes = 1; break;
		case 0x90:	printf("SUB    B"); opbytes = 1; break;
		case 0x91:	printf("SUB    C"); opbytes = 1; break;
		case 0x92:	printf("SUB    D"); opbytes = 1; break;
		case 0x93:	printf("SUB    E"); opbytes = 1; break;
		case 0x94:	printf("SUB    H"); opbytes = 1; break;
		case 0x95:	printf("SUB    L"); opbytes = 1; break;
		case 0x96:	printf("SUB    M"); opbytes = 1; break;
		case 0x97:	printf("SUB    A"); opbytes = 1; break;
		case 0x98:	printf("SBB    B"); opbytes = 1; break;
		case 0x99:	printf("SBB    C"); opbytes = 1; break;
		case 0x9a:	printf("SBB    D"); opbytes = 1; break;
		case 0x9b:	printf("SBB    E"); opbytes = 1; break;
		case 0x9c:	printf("SBB    H"); opbytes = 1; break;
		case 0x9d:	printf("SBB    L"); opbytes = 1; break;
		case 0x9e:	printf("SBB    M"); opbytes = 1; break;
		case 0x9f:	printf("SBB    A"); opbytes = 1; break;
		case 0xa0:	printf("ANA    B"); opbytes = 1; break;
		case 0xa1:	printf("ANA    C"); opbytes = 1; break;
		case 0xa2:	printf("ANA    D"); opbytes = 1; break;
		case 0xa3:	printf("ANA    E"); opbytes = 1; break;
		case 0xa4:	printf("ANA    H"); opbytes = 1; break;
		case 0xa5:	printf("ANA    L"); opbytes = 1; break;
		case 0xa6:	printf("ANA    M"); opbytes = 1; break;
		case 0xa7:	printf("ANA    A"); opbytes = 1; break;
		case 0xa8:	printf("XRA    B"); opbytes = 1; break;
		case 0xa9:	printf("XRA    C"); opbytes = 1; break;
		case 0xaa:	printf("XRA    D"); opbytes = 1; break;
		case 0xab:	printf("XRA    E"); opbytes = 1; break;
		case 0xac:	printf("XRA    H"); opbytes = 1; break;
		case 0xad:	printf("XRA    L"); opbytes = 1; break;
		case 0xae:	printf("XRA    M"); opbytes = 1; break;
		case 0xaf:	printf("XRA    A"); opbytes = 1; break;
		case 0xb0:	printf("ORA    B"); opbytes = 1; break;
		case 0xb1:	printf("ORA    C"); opbytes = 1; break;
		case 0xb2:	printf("ORA    D"); opbytes = 1; break;
		case 0xb3:	printf("ORA    E"); opbytes = 1; break;
		case 0xb4:	printf("ORA    H"); opbytes = 1; break;
		case 0xb5:	printf("ORA    L"); opbytes = 1; break;
		case 0xb6:	printf("ORA    M"); opbytes = 1; break;
		case 0xb7:	printf("ORA    A"); opbytes = 1; break;
		case 0xb8:	printf("CMP    B"); opbytes = 1; break;
		case 0xb9:	printf("CMP    C"); opbytes = 1; break;
		case 0xba:	printf("CMP    D"); opbytes = 1; break;
		case 0xbb:	printf("CMP    E"); opbytes = 1; break;
		case 0xbc:	printf("CMP    H"); opbytes = 1; break;
		case 0xbd:	printf("CMP    L"); opbytes = 1; break;
		case 0xbe:	printf("CMP    M"); opbytes = 1; break;
		case 0xbf:	printf("CMP    A"); opbytes = 1; break;
		case 0xc0:	printf("RNZ"); opbytes = 1; break;
		case 0xc1:	printf("POP    B"); opbytes = 1; break;
		case 0xc2:	printf("JNZ    adr,#$%02x%02x", codeLine[2], codeLine[1]); opbytes = 3; break;
		case 0xc3:	printf("JMP    adr,#$%02x%02x", codeLine[2], codeLine[1]); opbytes = 3; break;
		case 0xc4:	printf("CNZ    adr,#$%02x%02x", codeLine[2], codeLine[1]); opbytes = 3; break; 
		case 0xc5:	printf("PUSH   B"); opbytes = 1; break;
		case 0xc6:	printf("ADI    D8,#$%02x", codeLine[1]); opbytes = 2; break;
		case 0xc7:	printf("RST"); opbytes = 1; break;
		case 0xc8:	printf("RZ"); opbytes = 1; break;
		case 0xc9:	printf("RET"); opbytes = 1; break;
		case 0xca:	printf("JZ     adr,#$%02x%02x", codeLine[2], codeLine[1]); opbytes = 3; break; 
		case 0xcb:	printf("JMP    adr,#$%02x%02x", codeLine[2], codeLine[1]); opbytes = 3; break;
		case 0xcc:	printf("CZ     adr,#$%02x%02x", codeLine[2], codeLine[1]); opbytes = 3; break; 
		case 0xcd:	printf("CALL   adr,#$%02x%02x", codeLine[2], codeLine[1]); opbytes = 3; break; 
		case 0xce:	printf("ACI    D8,#$%02x", codeLine[1]); opbytes = 2; break;
		case 0xcf:	printf("RST"); opbytes = 1; break;
		case 0xd0:	printf("RNC"); opbytes = 1; break;
		case 0xd1:	printf("POP    D"); opbytes = 1; break;
		case 0xd2:	printf("JNC    adr,#$%02x%02x", codeLine[2], codeLine[1]); opbytes = 3; break;
		case 0xd3:	printf("OUT    D8,#$%02x", codeLine[1]); opbytes = 2; break;
		case 0xd4:	printf("CNC    adr,#$%02x%02x", codeLine[2], codeLine[1]); opbytes = 3; break;
		case 0xd5:	printf("PUSH   D"); opbytes = 1; break;
		case 0xd6:	printf("SUI    D8,#$%02x", codeLine[1]); opbytes = 2; break;
		case 0xd7:	printf("RST,#$%02x", codeLine[1]); opbytes = 2; break;
		case 0xd8:	printf("RC"); opbytes = 1; break;
		case 0xd9:	printf("RET"); opbytes = 1; break;
		case 0xda:	printf("JC     adr,#$%02x%02x", codeLine[2], codeLine[1]); opbytes = 3; break;
		case 0xdb:	printf("IN     D8,#$%02x", codeLine[1]); opbytes = 2; break;
		case 0xdc:	printf("CC     adr,#$%02x%02x", codeLine[2], codeLine[1]); opbytes = 3; break;
		case 0xdd:	printf("CALL   adr,#$%02x%02x", codeLine[2], codeLine[1]); opbytes = 3; break; 
		case 0xde:	printf("SBI    D8,#$%02x", codeLine[1]); opbytes = 2; break;
		case 0xdf:	printf("RST"); opbytes = 1; break;
		case 0xe0:	printf("RPO"); opbytes = 1; break;
		case 0xe1:	printf("POP    H"); opbytes = 1; break;
		case 0xe2:	printf("JPO    adr,#$%02x%02x", codeLine[2], codeLine[1]); opbytes = 3; break;
		case 0xe3:	printf("XTHL"); opbytes = 1; break;
		case 0xe4:	printf("CPO    adr,#$%02x%02x", codeLine[2], codeLine[1]); opbytes = 3; break;
		case 0xe5:	printf("PUSH   H"); opbytes = 1; break;
		case 0xe6:	printf("ANI    D8,#$%02x", codeLine[1]); opbytes = 2; break;
		case 0xe7:	printf("RST"); opbytes = 1; break;
		case 0xe8:	printf("RPE"); opbytes = 1; break;
		case 0xe9:	printf("PCHL"); opbytes = 1; break;
		case 0xea:	printf("JPE    adr,#$%02x%02x", codeLine[2], codeLine[1]); opbytes = 3; break;
		case 0xeb:	printf("XCHG"); opbytes = 1; break;
		case 0xec:	printf("CPE    adr,#$%02x%02x", codeLine[2], codeLine[1]); opbytes = 3; break;
		case 0xed:	printf("CALL   adr,#$%02x%02x", codeLine[2], codeLine[1]); opbytes = 3; break; 
		case 0xee:	printf("XRI    D8,#$%02x", codeLine[1]); opbytes = 2; break;
		case 0xef:	printf("RST"); opbytes = 1; break;
		case 0xf0:	printf("RP"); opbytes = 1; break;
		case 0xf1:	printf("POP    PSW"); opbytes = 1; break;
		case 0xf2:	printf("JP     adr,#$%02x%02x", codeLine[2], codeLine[1]); opbytes = 3; break;
		case 0xf3:	printf("DI"); opbytes = 1; break;
		case 0xf4:	printf("CP     adr,#$%02x%02x", codeLine[2], codeLine[1]); opbytes = 3; break;
		case 0xf5:	printf("PUSH   PSW"); opbytes = 1; break;
		case 0xf6:	printf("ORI    D8,#$%02x", codeLine[1]); opbytes = 2; break;
		case 0xf7:	printf("RST"); opbytes = 1; break;
		case 0xf8:	printf("RM"); opbytes = 1; break;
		case 0xf9:	printf("SPHL"); opbytes = 1; break;
		case 0xfa:	printf("JM     adr,#$%02x%02x", codeLine[2], codeLine[1]); opbytes = 3; break;
		case 0xfb:	printf("EI"); opbytes = 1; break;
		case 0xfc:	printf("CM     adr,#$%02x%02x", codeLine[2], codeLine[1]); opbytes = 3; break;
		case 0xfd:	printf("CALL   adr,#$%02x%02x", codeLine[2], codeLine[1]); opbytes = 3; break; 
		case 0xfe:	printf("CPI    D8,#$%02x", codeLine[1]); opbytes = 2; break;
		case 0xff:	printf("RST"); opbytes = 1; break;
	}
	state->pc++;
}
