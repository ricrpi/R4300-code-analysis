/*
 * mips.c
 *
 *  Created on: 24 Mar 2014
 *      Author: ric_rpi
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "memory.h"
#include "InstructionSetMIPS4.h"

#define INDEX "%08x"

#define I_OP "s%u, s%u, #%d"
#define OP_I(val) \
		((val & 0x03E00000) >> 21), \
		((val & 0x001F0000) >> 16), \
		(int)(val<<16)/(1<<16)

// Technically these are I OPs but I want to format the output differently
#define B_OP "r%u, s%u, #%d => 0x%08x, \traw 0x%08x"
#define OP_B(val) \
		((val & 0x03E00000) >> 21), \
		((val & 0x001F0000) >> 16), \
		(int)(val<<16)/(1<<16), \
		(x + 4 + (int)(val<<16)/(1<<16) * 4), \
		(val)// calculate offset from current position

// Technically these are I OPs but I want to format the output differently
#define BV_OP "r%u, 0x%02x, #%d => 0x%08x, \traw 0x%08x"
#define OP_BV(val) \
		((val & 0x03E00000) >> 21), \
		((val & 0x001F0000) >> 16), \
		(int)(val<<16)/(1<<16), \
		(x + 4 + (int)(val<<16)/(1<<16) * 4), \
		(val)// calculate offset from current position


#define J_OP "0x%08x // PC = 0x%08x, \traw 0x%08x"
#define OP_J(val) \
		(val & 0x03FFFFFF), \
		((x+8) & 0xF0000000) + ((val & 0x03FFFFFF)*4), \
		(val)

#define R_OP ""
#define OP_R(val) (val)

#define L_OP "r%u, [r%u, #%d]"
#define OP_L(val) \
		((val>>21)&0x1f), \
		((val>>16)&0x1f), \
		((int)(val<<16)/(1<<16))


static uint32_t mip_ops[sizeof_mips_op_t];

void count_ops(uint32_t* data, uint32_t len)
{
	memset(mip_ops, 0, sizeof_mips_op_t);

	int x;

	for(x=0;x<len;x++)
	{
		mip_ops[STRIP(ops_type(data[x]))]++;
	}
}


int32_t ops_JumpAddressOffset(uint32_t uiMIPSword)
{
	uint32_t op=uiMIPSword>>26;
	uint32_t op2;

	switch(op)
	{
	case 0x00: //printf("special\n");
		op2=uiMIPSword&0x3f;

		switch(op2)
		{
		case 0x08: return 0; // JR; 	//cannot return whats in a register
		case 0x09: return 0; // JALR;	//cannot return whats in a register
		} break;

	case 0x01:
		op2=(uiMIPSword>>16)&0x1f;
		switch(op2)
		{
		case 0x00: 	return (int32_t)(uiMIPSword<<16)/(1<<16); //printf(INDEX "\tBLTZ   \t" B_OP "\n",x, OP_B(val)); return;	// I
		case 0x01: 	return (int32_t)(uiMIPSword<<16)/(1<<16); //printf(INDEX "\tBGEZ   \t" B_OP "\n",x, OP_B(val)); return;	// I
		case 0x02: 	return (int32_t)(uiMIPSword<<16)/(1<<16); //printf(INDEX "\tBLTZL\n",x); 	return;
		case 0x03: 	return (int32_t)(uiMIPSword<<16)/(1<<16); //printf(INDEX "\tBGEZL\n",x); 	return;

		//case 0x09:  return (val<<16)/(1<<16); //printf(INDEX "\tBLTZAL \t" B_OP "\n",x, OP_B(val)); return;	// I and link
		case 0x10:  return (int32_t)(uiMIPSword<<16)/(1<<16); //printf(INDEX "\tBLTZAL \t" B_OP "\n",x, OP_B(val)); return;	// I and link
		case 0x11: 	return (int32_t)(uiMIPSword<<16)/(1<<16); //printf(INDEX "\tBGEZAL \t" B_OP "\n",x, OP_B(val)); return;	// I and link
		case 0x12: 	return (int32_t)(uiMIPSword<<16)/(1<<16); //printf(INDEX "\tBLTZALL\t" B_OP "\n",x, OP_B(val)); return;	// I and link likely
		case 0x13: 	return (int32_t)(uiMIPSword<<16)/(1<<16); //printf(INDEX "\tBGEZALL\t" B_OP "\n",x, OP_B(val)); return;	// I and link likely
		}break;

	case 0x02: 	return (uiMIPSword<<6)/(1<<6); //printf(INDEX "\tJ      \t" J_OP "\n", x, OP_J(val)); return;	// J
	case 0x03: 	return (uiMIPSword<<6)/(1<<6);   //printf(INDEX "\tJAL    \t" J_OP "\n", x, OP_J(val)); return;	// J
	case 0x04: 	return (int32_t)(uiMIPSword<<16)/(1<<16); //printf(INDEX "\tBEQ    \t" I_OP "\n", x, OP_I(val)); return;	// I
	case 0x05: 	return (int32_t)(uiMIPSword<<16)/(1<<16); //printf(INDEX "\tBNE    \t" I_OP "\n", x, OP_I(val)); return;	// I
	case 0x06: 	return (int32_t)(uiMIPSword<<16)/(1<<16); //printf(INDEX "\tBLEZ   \t\n", x ); return;
	case 0x07: 	return (int32_t)(uiMIPSword<<16)/(1<<16); //printf(INDEX "\tBGTZ   \t\n", x ); return;

	case 0x14: return (int32_t)(uiMIPSword<<16)/(1<<16); //printf(INDEX "\tBEQL  \n",x); return;
	case 0x15: return (int32_t)(uiMIPSword<<16)/(1<<16); //printf(INDEX "\tBNEL  \n",x); return;
	case 0x16: return (int32_t)(uiMIPSword<<16)/(1<<16); //printf(INDEX "\tBLEZL \n",x); return;
	case 0x17: return (int32_t)(uiMIPSword<<16)/(1<<16); //printf(INDEX "\tBGTZL \n",x); return;
	}
	printf("%d invalid\n",__LINE__);
	return 0x7FFFFFF;
}

uint64_t ops_regs_used(uint32_t uiMIPSword)
{
	uint32_t op=uiMIPSword>>26;
	uint32_t op2;

	//TODO need to return registers
	switch(op)
	{
		case 0x00:
			op2=uiMIPSword&0x3f;
			switch(op2)
			{
				case 0x00: return MIPS_REG((uiMIPSword>>16)&0x1f) | MIPS_REG((uiMIPSword>>11)&0x1f); // SLL;
				case 0x02: return MIPS_REG_ALL; // SRL;
				case 0x03: return MIPS_REG_ALL; // SRA;
				case 0x04: return MIPS_REG_ALL; // SLLV;
				case 0x07: return MIPS_REG_ALL; // SRAV;
				case 0x08: return MIPS_REG_ALL; // JR;
				case 0x09: return MIPS_REG_ALL; // JALR;
				case 0x0C: return MIPS_REG_ALL; // SYSCALL;
				case 0x0D: return MIPS_REG_ALL; // BREAK;
				case 0x0F: return MIPS_REG_ALL; // SYNC;
				case 0x10: return MIPS_REG_ALL; // MFHI;
				case 0x11: return MIPS_REG_ALL; // MTHI;
				case 0x12: return MIPS_REG_ALL; // MFLO;
				case 0x13: return MIPS_REG_ALL; // MTLO;
				case 0x14: return MIPS_REG_ALL; // DSLLV;
				case 0x16: return MIPS_REG_ALL; // DSRLV;
				case 0x17: return MIPS_REG_ALL; // DSRAV;
				case 0x18: return MIPS_REG_ALL; // MULT;
				case 0x19: return MIPS_REG_ALL; // MULTU;
				case 0x1A: return MIPS_REG_ALL; // DIV;
				case 0x1B: return MIPS_REG_ALL; // DIVU;
				case 0x1C: return MIPS_REG_ALL; // DMULT;
				case 0x1D: return MIPS_REG_ALL; // DMULTU;
				case 0x1E: return MIPS_REG_ALL; // DDIV;
				case 0x1F: return MIPS_REG_ALL; // DDIVU;
				case 0x20: return MIPS_REG((uiMIPSword>>21)&0x1f) | MIPS_REG((uiMIPSword>>16)&0x1f) | MIPS_REG((uiMIPSword>>11)&0x1f); // ADD;
				case 0x21: return MIPS_REG_ALL; // ADDU;
				case 0x22: return MIPS_REG_ALL; // SUB;
				case 0x23: return MIPS_REG_ALL; // SUBU;
				case 0x24: return MIPS_REG_ALL; // AND;
				case 0x25: return MIPS_REG_ALL; // OR;
				case 0x26: return MIPS_REG_ALL; // XOR;
				case 0x27: return MIPS_REG_ALL; // NOR;
				case 0x2A: return MIPS_REG_ALL; // SLT;
				case 0x2B: return MIPS_REG_ALL; // SLTU;
				case 0x2C: return MIPS_REG_ALL; // DADD;
				case 0x2D: return MIPS_REG_ALL; // DADDU;
				case 0x2E: return MIPS_REG_ALL; // DSUB;
				case 0x2F: return MIPS_REG_ALL; // DSUBU;
				case 0x30: return MIPS_REG_ALL; // TGE;
				case 0x31: return MIPS_REG_ALL; // TGEU;
				case 0x32: return MIPS_REG_ALL; // TLT;
				case 0x33: return MIPS_REG_ALL; // TLTU;
				case 0x34: return MIPS_REG_ALL; // TEQ;
				case 0x36: return MIPS_REG_ALL; // TNE;
				case 0x38: return MIPS_REG_ALL; // DSLL;
				case 0x3A: return MIPS_REG_ALL; // DSRL;
				case 0x3B: return MIPS_REG_ALL; // DSRA;
				case 0x3C: return MIPS_REG_ALL; // DSLL32;
				case 0x3E: return MIPS_REG_ALL; // DSRL32;
				case 0x3F: return MIPS_REG_ALL; // DSRA32;
			}
			break;
		case 0x01:
			op2=(uiMIPSword>>16)&0x1f;
			switch(op2)
			{
			case 0x00: 	return MIPS_REG_ALL; // BLTZ;	// I
			case 0x01: 	return MIPS_REG_ALL; // BGEZ;	// I
			case 0x02: 	return MIPS_REG_ALL; // BLTZL;
			case 0x03: 	return MIPS_REG_ALL; // BGEZL;
			case 0x08: 	return MIPS_REG_ALL; // TGEI;
			case 0x09: 	return MIPS_REG_ALL; // TGEIU;
			case 0x0A: 	return MIPS_REG_ALL; // TLTI;
			case 0x0B: 	return MIPS_REG_ALL; // TLTIU;
			case 0x0C: 	return MIPS_REG_ALL; // TEQI;
			case 0x0E: 	return MIPS_REG_ALL; // TNEI;
			case 0x10: 	return MIPS_REG_ALL; // BLTZAL;	// I and link
			case 0x11: 	return MIPS_REG_ALL; // BGEZAL;	// I and link
			case 0x12: 	return MIPS_REG_ALL; // BLTZALL;	// I and link likely
			case 0x13: 	return MIPS_REG_ALL; // BGEZALL;	// I and link likely
			}

			break;

		case 0x02: 	return 0; //J;
		case 0x03: 	return 0; //JAL;
		case 0x04: 	return MIPS_REG_ALL; // BEQ;	// I
		case 0x05: 	return MIPS_REG((uiMIPSword>>21)&0x1f) | MIPS_REG((uiMIPSword>>16)&0x1f); // BNE;	// I
		case 0x06: 	return MIPS_REG_ALL; // BLEZ;
		case 0x07: 	return MIPS_REG_ALL; // BGTZ;
		case 0x08: 	return MIPS_REG((uiMIPSword>>21)&0x1f) | MIPS_REG((uiMIPSword>>16)&0x1f); // ADDI;	// I
		case 0x09: 	return MIPS_REG((uiMIPSword>>21)&0x1f) | MIPS_REG((uiMIPSword>>16)&0x1f); // ADDIU;	// I
		case 0x0A: 	return MIPS_REG_ALL; // SLTI;	// I
		case 0x0B: 	return MIPS_REG_ALL; // SLTIU;	// I
		case 0x0C: 	return MIPS_REG_ALL; // ANDI; 	// I
		case 0x0D: 	return MIPS_REG((uiMIPSword>>21)&0x1f) | MIPS_REG((uiMIPSword>>16)&0x1f); // ORI;	// I
		case 0x0E: 	return MIPS_REG((uiMIPSword>>21)&0x1f) | MIPS_REG((uiMIPSword>>16)&0x1f); // XORI;	// I
		case 0x0F: 	return MIPS_REG_ALL; // LUI;	// I
		case 0x10: 	//return cop0\n",x);
			op2=(uiMIPSword>>21)&0x1f;
			switch(op2)
			{
			case 0x00: return MIPS_REG_ALL; // MFC0;
			case 0x04: return MIPS_REG_ALL; // MTC0;
			case 0x10: //return tlb;
				switch(uiMIPSword&0x3f)
				{
				case 0x01: return MIPS_REG_ALL; // TLBR;
				case 0x02: return MIPS_REG_ALL; // TLBWI;
				case 0x06: return MIPS_REG_ALL; // TLBWR;
				case 0x08: return MIPS_REG_ALL; // TLBP;
				case 0x18: return MIPS_REG_ALL; // ERET;
				}
			}
			break;

		case 0x11: //return cop1\n",x);
			op2=(uiMIPSword>>21)&0x1f;
			switch(op2)
			{
			case 0x00: return MIPS_REG_ALL; // MFC1;
			case 0x01: return MIPS_REG_ALL; // DMFC1;
			case 0x02: return MIPS_REG_ALL; // CFC1;
			case 0x04: return MIPS_REG_ALL; // MTC1;
			case 0x05: return MIPS_REG_ALL; // DMTC1;
			case 0x06: return MIPS_REG_ALL; // CTC1;
			case 0x08: //return BC1;
				switch((uiMIPSword>>16)&0x3)
				{
				case 0x00: return MIPS_REG_ALL; // BC1F;
				case 0x01: return MIPS_REG_ALL; // BC1T;
				case 0x02: return MIPS_REG_ALL; // BC1FL;
				case 0x03: return MIPS_REG_ALL; // BC1TL;
				}break;

			case 0x10: //return C1.S\n",x);
				switch(uiMIPSword&0x3f)
				{
				case 0x00: return MIPS_REG_ALL; // ADD_S;
				case 0x01: return MIPS_REG_ALL; // SUB_S;
				case 0x02: return MIPS_REG_ALL; // MUL_S;
				case 0x03: return MIPS_REG_ALL; // DIV_S;
				case 0x04: return MIPS_REG_ALL; // SQRT_S;
				case 0x05: return MIPS_REG_ALL; // ABS_S;
				case 0x06: return MIPS_REG_ALL; // MOV_S;
				case 0x07: return MIPS_REG_ALL; // NEG_S;
				case 0x08: return MIPS_REG_ALL; // ROUND_L_S;
				case 0x09: return MIPS_REG_ALL; // TRUNC_L_S;
				case 0x0A: return MIPS_REG_ALL; // CEIL_L_S;
				case 0x0B: return MIPS_REG_ALL; // FLOOR_L_S;
				case 0x0C: return MIPS_REG_ALL; // ROUND_W_S;
				case 0x0D: return MIPS_REG_ALL; // TRUNC_W_S;
				case 0x0E: return MIPS_REG_ALL; // CEIL_W_S;
				case 0x0F: return MIPS_REG_ALL; // FLOOR_W_S;
				case 0x21: return MIPS_REG_ALL; // CVT_D_S;
				case 0x24: return MIPS_REG_ALL; // CVT_W_S;
				case 0x25: return MIPS_REG_ALL; // CVT_L_S;
				case 0x30: return MIPS_REG_ALL; // C_F_S;
				case 0x31: return MIPS_REG_ALL; // C_UN_S;
				case 0x32: return MIPS_REG_ALL; // C_EQ_S;
				case 0x33: return MIPS_REG_ALL; // C_UEQ_S;
				case 0x34: return MIPS_REG_ALL; // C_OLT_S;
				case 0x35: return MIPS_REG_ALL; // C_ULT_S;
				case 0x36: return MIPS_REG_ALL; // C_OLE_S;
				case 0x37: return MIPS_REG_ALL; // C_ULE_S;
				case 0x38: return MIPS_REG_ALL; // C_SF_S;
				case 0x39: return MIPS_REG_ALL; // C_NGLE_S;
				case 0x3A: return MIPS_REG_ALL; // C_SEQ_S;
				case 0x3B: return MIPS_REG_ALL; // C_NGL_S;
				case 0x3C: return MIPS_REG_ALL; // C_LT_S;
				case 0x3D: return MIPS_REG_ALL; // C_NGE_S;
				case 0x3E: return MIPS_REG_ALL; // C_LE_S;
				case 0x3F: return MIPS_REG_ALL; // C_NGT_S;
				}break;
			case 0x11: //return C1_D\n",x);
				switch(uiMIPSword&0x3f)
				{
				case 0x00: return MIPS_REG_ALL; // ADD_D;
				case 0x01: return MIPS_REG_ALL; // SUB_D;
				case 0x02: return MIPS_REG_ALL; // MUL_D;
				case 0x03: return MIPS_REG_ALL; // DIV_D;
				case 0x04: return MIPS_REG_ALL; // SQRT_D;
				case 0x05: return MIPS_REG_ALL; // ABS_D;
				case 0x06: return MIPS_REG_ALL; // MOV_D;
				case 0x07: return MIPS_REG_ALL; // NEG_D;
				case 0x08: return MIPS_REG_ALL; // ROUND_L_D;
				case 0x09: return MIPS_REG_ALL; // TRUNC_L_D;
				case 0x0A: return MIPS_REG_ALL; // CEIL_L_D;
				case 0x0B: return MIPS_REG_ALL; // FLOOR_L_D;
				case 0x0C: return MIPS_REG_ALL; // ROUND_W_D;
				case 0x0D: return MIPS_REG_ALL; // TRUNC_W_D;
				case 0x0E: return MIPS_REG_ALL; // CEIL_W_D;
				case 0x0F: return MIPS_REG_ALL; // FLOOR_W_D;
				case 0x20: return MIPS_REG_ALL; // CVT_S_D;
				case 0x24: return MIPS_REG_ALL; // CVT_W_D;
				case 0x25: return MIPS_REG_ALL; // CVT_L_D;
				case 0x30: return MIPS_REG_ALL; // C_F_D;
				case 0x31: return MIPS_REG_ALL; // C_UN_D;
				case 0x32: return MIPS_REG_ALL; // C_EQ_D;
				case 0x33: return MIPS_REG_ALL; // C_UEQ_D;
				case 0x34: return MIPS_REG_ALL; // C_OLT_D;
				case 0x35: return MIPS_REG_ALL; // C_ULT_D;
				case 0x36: return MIPS_REG_ALL; // C_OLE_D;
				case 0x37: return MIPS_REG_ALL; // C_ULE_D;
				case 0x38: return MIPS_REG_ALL; //  C_SF_D;
				case 0x39: return MIPS_REG_ALL; // C_NGLE_D;
				case 0x3A: return MIPS_REG_ALL; // C_SEQ_D;
				case 0x3B: return MIPS_REG_ALL; // C_NGL_D;
				case 0x3C: return MIPS_REG_ALL; // C_LT_D;
				case 0x3D: return MIPS_REG_ALL; // C_NGE_D;
				case 0x3E: return MIPS_REG_ALL; // C_LE_D;
				case 0x3F: return MIPS_REG_ALL; // C_NGT_D;
				} break;
			case 0x14: //return C1_W\n",x);
				switch(uiMIPSword&0x3f)
				{
				case 0x20: return MIPS_REG_ALL; // CVT_S_W;
				case 0x21: return MIPS_REG_ALL; // CVT_D_W;
				}
				break;

			case 0x15: //return C1_L\n",x);
				switch(uiMIPSword&0x3f)
				{
				case 0x20: return MIPS_REG_ALL; // CVT_S_L;
				case 0x21: return MIPS_REG_ALL; // CVT_D_L;
				}
				break;
			}break;

		case 0x14: return MIPS_REG_ALL; // BEQL;
		case 0x15: return MIPS_REG((uiMIPSword>>21)&0x1f) | MIPS_REG((uiMIPSword>>16)&0x1f); // BNEL;
		case 0x16: return MIPS_REG_ALL; // BLEZL;
		case 0x17: return MIPS_REG_ALL; // BGTZL;
		case 0x18: return MIPS_REG_ALL; // DADDI;
		case 0x19: return MIPS_REG_ALL; // DADDIU;
		case 0x1A: return MIPS_REG_ALL; // LDL;
		case 0x1B: return MIPS_REG_ALL; // LDR;
		case 0x20: return MIPS_REG_ALL; // LB;	// Load Byte
		case 0x21: return MIPS_REG_ALL; // LH;	// Load Halfword
		case 0x22: return MIPS_REG_ALL; // LWL;
		case 0x23: return MIPS_REG((uiMIPSword>>21)&0x1f) | MIPS_REG((uiMIPSword>>16)&0x1f); // LW;	// Load Word
		case 0x24: return MIPS_REG_ALL; // LBU;	// Load Unsigned Byte
		case 0x25: return MIPS_REG_ALL; // LHU;	// Load Halfword unsigned
		case 0x26: return MIPS_REG_ALL; // LWR;
		case 0x27: return MIPS_REG_ALL; // LWU;	// Load Word unsigned
		case 0x28: return MIPS_REG_ALL; // SB;	// I
		case 0x29: return MIPS_REG_ALL; // SH;	// I
		case 0x2A: return MIPS_REG_ALL; // SWL;
		case 0x2B: return MIPS_REG((uiMIPSword>>21)&0x1f) | MIPS_REG((uiMIPSword>>16)&0x1f); // SW;	// I
		case 0x2C: return MIPS_REG_ALL; // SDL;
		case 0x2D: return MIPS_REG_ALL; // SDR;
		case 0x2E: return MIPS_REG_ALL; // SWR;
		case 0x2F: return MIPS_REG_ALL; // CACHE;
		case 0x30: return MIPS_REG_ALL; // LL;	// Load Linked Word atomic Read-Modify-Write ops
		case 0x31: return MIPS_REG_ALL; // LWC1;	// Load Word to co processor 1
		case 0x34: return MIPS_REG_ALL; // LLD;	// Load Linked Dbl Word atomic Read-Modify-Write ops
		case 0x35: return MIPS_REG_ALL; // LDC1;
		case 0x37: return MIPS_REG_ALL; // LD; 	// Load Double word
		case 0x38: return MIPS_REG_ALL; // SC;	// Store Linked Word atomic Read-Modify-Write ops
		case 0x39: return MIPS_REG_ALL; // SWC1;	// Store Word from co processor 1 to memory
		case 0x3C: return MIPS_REG_ALL; // SCD;	// Store Conditional Double Word
		case 0x3D: return MIPS_REG_ALL; // SDC1;
		case 0x3F: return MIPS_REG_ALL; // SD; 	// Store Double word
	}

	return MIPS_REG_ALL;
}

mips_op_t ops_type(uint32_t uiMIPSword)
{
	uint32_t op=uiMIPSword>>26;
	uint32_t op2;

	switch(op)
	{
		case 0x00:
	        op2=uiMIPSword&0x3f;
	        switch(op2)
	        {
	          case 0x00: return SLL;
	          case 0x02: return SRL;
	          case 0x03: return SRA;
	          case 0x04: return SLLV;
	          case 0x07: return SRAV;
	          case 0x08: return JR;
	          case 0x09: return JALR;
	          case 0x0C: return SYSCALL;
	          case 0x0D: return BREAK;
	          case 0x0F: return SYNC;
	          case 0x10: return MFHI;
	          case 0x11: return MTHI;
	          case 0x12: return MFLO;
	          case 0x13: return MTLO;
	          case 0x14: return DSLLV;
	          case 0x16: return DSRLV;
	          case 0x17: return DSRAV;
	          case 0x18: return MULT;
	          case 0x19: return MULTU;
	          case 0x1A: return DIV;
	          case 0x1B: return DIVU;
	          case 0x1C: return DMULT;
	          case 0x1D: return DMULTU;
	          case 0x1E: return DDIV;
	          case 0x1F: return DDIVU;
	          case 0x20: return ADD;
	          case 0x21: return ADDU;
	          case 0x22: return SUB;
	          case 0x23: return SUBU;
	          case 0x24: return AND;
	          case 0x25: return OR;
	          case 0x26: return XOR;
	          case 0x27: return NOR;
	          case 0x2A: return SLT;
	          case 0x2B: return SLTU;
	          case 0x2C: return DADD;
	          case 0x2D: return DADDU;
	          case 0x2E: return DSUB;
	          case 0x2F: return DSUBU;
	          case 0x30: return TGE;
	          case 0x31: return TGEU;
	          case 0x32: return TLT;
	          case 0x33: return TLTU;
	          case 0x34: return TEQ;
	          case 0x36: return TNE;
	          case 0x38: return DSLL;
	          case 0x3A: return DSRL;
	          case 0x3B: return DSRA;
	          case 0x3C: return DSLL32;
	          case 0x3E: return DSRL32;
	          case 0x3F: return DSRA32;
	        }
	        break;

	case 0x01:
		op2=(uiMIPSword>>16)&0x1f;
		switch(op2)
		{
		case 0x00: 	return BLTZ;	// I
		case 0x01: 	return BGEZ;	// I
		case 0x02: 	return BLTZL;
		case 0x03: 	return BGEZL;
		case 0x08: 	return TGEI;
		case 0x09: 	return TGEIU;
		case 0x0A: 	return TLTI;
		case 0x0B: 	return TLTIU;
		case 0x0C: 	return TEQI;
		case 0x0E: 	return TNEI;
		case 0x10: 	return BLTZAL;	// I and link
		case 0x11: 	return BGEZAL;	// I and link
		case 0x12: 	return BLTZALL;	// I and link likely
		case 0x13: 	return BGEZALL;	// I and link likely
		}

		break;

	case 0x02: 	return J;	// J
	case 0x03: 	return JAL; // J
	case 0x04: 	return BEQ;	// I
	case 0x05: 	return BNE;	// I
	case 0x06: 	return BLEZ;
	case 0x07: 	return BGTZ;
	case 0x08: 	return ADDI;	// I
	case 0x09: 	return ADDIU;	// I
	case 0x0A: 	return SLTI;	// I
	case 0x0B: 	return SLTIU;	// I
	case 0x0C: 	return ANDI; 	// I
	case 0x0D: 	return ORI;	// I
	case 0x0E: 	return XORI;	// I
	case 0x0F: 	return LUI;	// I
	case 0x10: 	//return cop0\n",x);
		op2=(uiMIPSword>>21)&0x1f;
		switch(op2)
		{
		case 0x00: return MFC0;
		case 0x04: return MTC0;
		case 0x10: //return tlb;
			switch(uiMIPSword&0x3f)
			{
			case 0x01: return TLBR;
			case 0x02: return TLBWI;
			case 0x06: return TLBWR;
			case 0x08: return TLBP;
			case 0x18: return ERET;
			}
		}
		break;

	case 0x11: //return cop1\n",x);
		op2=(uiMIPSword>>21)&0x1f;
		switch(op2)
		{
		case 0x00: return MFC1;
		case 0x01: return DMFC1;
		case 0x02: return CFC1;
		case 0x04: return MTC1;
		case 0x05: return DMTC1;
		case 0x06: return CTC1;
		case 0x08: //return BC1;
			switch((uiMIPSword>>16)&0x3)
			{
			case 0x00: return BC1F;
			case 0x01: return BC1T;
			case 0x02: return BC1FL;
			case 0x03: return BC1TL;
			}break;

		case 0x10: //return C1.S\n",x);
			switch(uiMIPSword&0x3f)
			{
			case 0x00: return ADD_S;
			case 0x01: return SUB_S;
			case 0x02: return MUL_S;
			case 0x03: return DIV_S;
			case 0x04: return SQRT_S;
			case 0x05: return ABS_S;
			case 0x06: return MOV_S;
			case 0x07: return NEG_S;
			case 0x08: return ROUND_L_S;
			case 0x09: return TRUNC_L_S;
			case 0x0A: return CEIL_L_S;
			case 0x0B: return FLOOR_L_S;
			case 0x0C: return ROUND_W_S;
			case 0x0D: return TRUNC_W_S;
			case 0x0E: return CEIL_W_S;
			case 0x0F: return FLOOR_W_S;
			case 0x21: return CVT_D_S;
			case 0x24: return CVT_W_S;
			case 0x25: return CVT_L_S;
			case 0x30: return C_F_S;
			case 0x31: return C_UN_S;
			case 0x32: return C_EQ_S;
			case 0x33: return C_UEQ_S;
			case 0x34: return C_OLT_S;
			case 0x35: return C_ULT_S;
			case 0x36: return C_OLE_S;
			case 0x37: return C_ULE_S;
			case 0x38: return C_SF_S;
			case 0x39: return C_NGLE_S;
			case 0x3A: return C_SEQ_S;
			case 0x3B: return C_NGL_S;
			case 0x3C: return C_LT_S;
			case 0x3D: return C_NGE_S;
			case 0x3E: return C_LE_S;
			case 0x3F: return C_NGT_S;
			}break;
		case 0x11: //return C1_D\n",x);
			switch(uiMIPSword&0x3f)
			{
			case 0x00: return ADD_D;
			case 0x01: return SUB_D;
			case 0x02: return MUL_D;
			case 0x03: return DIV_D;
			case 0x04: return SQRT_D;
			case 0x05: return ABS_D;
			case 0x06: return MOV_D;
			case 0x07: return NEG_D;
			case 0x08: return ROUND_L_D;
			case 0x09: return TRUNC_L_D;
			case 0x0A: return CEIL_L_D;
			case 0x0B: return FLOOR_L_D;
			case 0x0C: return ROUND_W_D;
			case 0x0D: return TRUNC_W_D;
			case 0x0E: return CEIL_W_D;
			case 0x0F: return FLOOR_W_D;
			case 0x20: return CVT_S_D;
			case 0x24: return CVT_W_D;
			case 0x25: return CVT_L_D;
			case 0x30: return C_F_D;
			case 0x31: return C_UN_D;
			case 0x32: return C_EQ_D;
			case 0x33: return C_UEQ_D;
			case 0x34: return C_OLT_D;
			case 0x35: return C_ULT_D;
			case 0x36: return C_OLE_D;
			case 0x37: return C_ULE_D;
			case 0x38: return C_SF_D;
			case 0x39: return C_NGLE_D;
			case 0x3A: return C_SEQ_D;
			case 0x3B: return C_NGL_D;
			case 0x3C: return C_LT_D;
			case 0x3D: return C_NGE_D;
			case 0x3E: return C_LE_D;
			case 0x3F: return C_NGT_D;
			} break;
		case 0x14: //return C1_W\n",x);
			switch(uiMIPSword&0x3f)
			{
			case 0x20: return CVT_S_W;
			case 0x21: return CVT_D_W;
			}
			break;

		case 0x15: //return C1_L\n",x);
			switch(uiMIPSword&0x3f)
			{
			case 0x20: return CVT_S_L;
			case 0x21: return CVT_D_L;
			}
			break;
		}break;

	case 0x14: return BEQL;
	case 0x15: return BNEL;
	case 0x16: return BLEZL;
	case 0x17: return BGTZL;
	case 0x18: return DADDI;
	case 0x19: return DADDIU;
	case 0x1A: return LDL;
	case 0x1B: return LDR;
	case 0x20: return LB;	// Load Byte
	case 0x21: return LH;	// Load Halfword
	case 0x22: return LWL;
	case 0x23: return LW;	// Load Word
	case 0x24: return LBU;	// Load Unsigned Byte
	case 0x25: return LHU;	// Load Halfword unsigned
	case 0x26: return LWR;
	case 0x27: return LWU;	// Load Word unsigned
	case 0x28: return SB;	// I
	case 0x29: return SH;	// I
	case 0x2A: return SWL;
	case 0x2B: return SW;	// I
	case 0x2C: return SDL;
	case 0x2D: return SDR;
	case 0x2E: return SWR;
	case 0x2F: return CACHE;
	case 0x30: return LL;	// Load Linked Word atomic Read-Modify-Write ops
	case 0x31: return LWC1;	// Load Word to co processor 1
	case 0x34: return LLD;	// Load Linked Dbl Word atomic Read-Modify-Write ops
	case 0x35: return LDC1;
	case 0x37: return LD; 	// Load Double word
	case 0x38: return SC;	// Store Linked Word atomic Read-Modify-Write ops
	case 0x39: return SWC1;	// Store Word from co processor 1 to memory
	case 0x3C: return SCD;	// Store Conditional Double Word
	case 0x3D: return SDC1;
	case 0x3F: return SD; 	// Store Double word
	}

	return INVALID;
}

void ops_decode(uint32_t x, uint32_t uiMIPSword)
{
	uint32_t op=uiMIPSword>>26;
	uint32_t op2;

	switch(op)
	{
	case 0x00:
		op2=uiMIPSword&0x3f;
		switch(op2)
		{
			case 0x00: printf(INDEX "\tSLL   \t" I_OP "\n", x, OP_I(uiMIPSword)); return;	// I
			case 0x02: printf(INDEX "\tSRL   \t" I_OP "\n", x, OP_I(uiMIPSword)); return;	// I
			case 0x03: printf(INDEX "\tSRA   \t" I_OP "\n", x, OP_I(uiMIPSword)); return;	// I
			case 0x04: printf(INDEX "\tSLLV  \t" I_OP "\n", x, OP_I(uiMIPSword)); return;	// I
			case 0x07: printf(INDEX "\tSRAV  \t" I_OP "\n", x, OP_I(uiMIPSword)); return;	// I
			case 0x08: printf(INDEX "\tJR    \tr%d\n", x, (uiMIPSword>>21)&0x1f); return;	// J
			case 0x09: printf(INDEX "\tJALR  \tr%d // PC=r%d, r%d=%d \n", x,
					(uiMIPSword>>21)&0x1f,
					(uiMIPSword>>21)&0x1f,
					(uiMIPSword>>11)&0x1f,
					x+8); return;	// J
			case 0x0C: printf(INDEX "\tSYSCALL\t" I_OP "\n", x, OP_I(uiMIPSword)); return;	// I
			case 0x0D: printf(INDEX "\tBREAK \t" I_OP "\n", x, OP_I(uiMIPSword)); return;	// I
			case 0x0F: printf(INDEX "\tSYNC  \t" I_OP "\n", x, OP_I(uiMIPSword)); return;	// I
			case 0x10: printf(INDEX "\tMFHI  \t" I_OP "\n", x, OP_I(uiMIPSword)); return;	// I
			case 0x11: printf(INDEX "\tMTHI  \t" I_OP "\n", x, OP_I(uiMIPSword)); return;	// I
			case 0x12: printf(INDEX "\tMFLO  \t" I_OP "\n", x, OP_I(uiMIPSword)); return;	// I
			case 0x13: printf(INDEX "\tMTLO  \t" I_OP "\n", x, OP_I(uiMIPSword)); return;	// I
			case 0x14: printf(INDEX "\tDSLLV \t" I_OP "\n", x, OP_I(uiMIPSword)); return;	// I
			case 0x16: printf(INDEX "\tDSRLV \t" I_OP "\n", x, OP_I(uiMIPSword)); return;	// I
			case 0x17: printf(INDEX "\tDSRAV \t" I_OP "\n", x, OP_I(uiMIPSword)); return;	// I
			case 0x18: printf(INDEX "\tMULT  \t" I_OP "\n", x, OP_I(uiMIPSword)); return;	// I
			case 0x19: printf(INDEX "\tMULTU \t" I_OP "\n", x, OP_I(uiMIPSword)); return;	// I
			case 0x1A: printf(INDEX "\tDIV   \t" I_OP "\n", x, OP_I(uiMIPSword)); return;	// I
			case 0x1B: printf(INDEX "\tDIVU  \t" I_OP "\n", x, OP_I(uiMIPSword)); return;	// I
			case 0x1C: printf(INDEX "\tDMULT \t" I_OP "\n", x, OP_I(uiMIPSword)); return;	// I
			case 0x1D: printf(INDEX "\tDMULTU\t" I_OP "\n", x, OP_I(uiMIPSword)); return;	// I
			case 0x1E: printf(INDEX "\tDDIV  \t" I_OP "\n", x, OP_I(uiMIPSword)); return;	// I
			case 0x1F: printf(INDEX "\tDDIVU \t" I_OP "\n", x, OP_I(uiMIPSword)); return;	// I
			case 0x20: printf(INDEX "\tADD   \t" I_OP "\n", x, OP_I(uiMIPSword)); return;	// I
			case 0x21: printf(INDEX "\tADDU  \t" I_OP "\n", x, OP_I(uiMIPSword)); return;	// I
			case 0x22: printf(INDEX "\tSUB   \t" I_OP "\n", x, OP_I(uiMIPSword)); return;	// I
			case 0x23: printf(INDEX "\tSUBU  \t" I_OP "\n", x, OP_I(uiMIPSword)); return;	// I
			case 0x24: printf(INDEX "\tAND   \t" I_OP "\n", x, OP_I(uiMIPSword)); return;	// I
			case 0x25: printf(INDEX "\tOR    \t" I_OP "\n", x, OP_I(uiMIPSword)); return;	// I
			case 0x26: printf(INDEX "\tXOR   \t" I_OP "\n", x, OP_I(uiMIPSword)); return;	// I
			case 0x27: printf(INDEX "\tNOR   \t" I_OP "\n", x, OP_I(uiMIPSword)); return;	// I
			case 0x2A: printf(INDEX "\tSLT   \t" I_OP "\n", x, OP_I(uiMIPSword)); return;	// I
			case 0x2B: printf(INDEX "\tSLTU  \t" I_OP "\n", x, OP_I(uiMIPSword)); return;	// I
			case 0x2C: printf(INDEX "\tDADD  \t" I_OP "\n", x, OP_I(uiMIPSword)); return;	// I
			case 0x2D: printf(INDEX "\tDADDU \t" I_OP "\n", x, OP_I(uiMIPSword)); return;	// I
			case 0x2E: printf(INDEX "\tDSUB  \t" I_OP "\n", x, OP_I(uiMIPSword)); return;	// I
			case 0x2F: printf(INDEX "\tDSUBU \t" I_OP "\n", x, OP_I(uiMIPSword)); return;	// I
			case 0x30: printf(INDEX "\tTGE   \t" I_OP "\n", x, OP_I(uiMIPSword)); return;	// I
			case 0x31: printf(INDEX "\tTGEU  \t" I_OP "\n", x, OP_I(uiMIPSword)); return;	// I
			case 0x32: printf(INDEX "\tTLT   \t" I_OP "\n", x, OP_I(uiMIPSword)); return;	// I
			case 0x33: printf(INDEX "\tTLTU  \t" I_OP "\n", x, OP_I(uiMIPSword)); return;	// I
			case 0x34: printf(INDEX "\tTEQ   \t" I_OP "\n", x, OP_I(uiMIPSword)); return;	// I
			case 0x36: printf(INDEX "\tTNE   \t" I_OP "\n", x, OP_I(uiMIPSword)); return;	// I
			case 0x38: printf(INDEX "\tDSLL  \t" I_OP "\n", x, OP_I(uiMIPSword)); return;	// I
			case 0x3A: printf(INDEX "\tDSRL  \t" I_OP "\n", x, OP_I(uiMIPSword)); return;	// I
			case 0x3B: printf(INDEX "\tDSRA  \t" I_OP "\n", x, OP_I(uiMIPSword)); return;	// I
			case 0x3C: printf(INDEX "\tDSLL32\t" I_OP "\n", x, OP_I(uiMIPSword)); return;	// I
			case 0x3E: printf(INDEX "\tDSRL32\t" I_OP "\n", x, OP_I(uiMIPSword)); return;	// I
			case 0x3F: printf(INDEX "\tDSRA32\t" I_OP "\n", x, OP_I(uiMIPSword)); return;	// I
		}
		break;

	case 0x01:
		op2=(uiMIPSword>>16)&0x1f;

		switch(op2)
		{
		case 0x00: 	printf(INDEX "\tBLTZ   \t" BV_OP "\n", x, OP_BV(uiMIPSword)); return;	// I
		case 0x01: 	printf(INDEX "\tBGEZ   \t" BV_OP "\n", x, OP_BV(uiMIPSword)); return;	// I
		case 0x02: 	printf(INDEX "\tBLTZL  \t" BV_OP "\n", x, OP_BV(uiMIPSword)); return;	// I
		case 0x03: 	printf(INDEX "\tBGEZL  \t" BV_OP "\n", x, OP_BV(uiMIPSword)); return;	// I
		case 0x08: 	printf(INDEX "\tTGEI\n",x); 	return;
		case 0x09: 	printf(INDEX "\tTGEIU\n",x); 	return;
		case 0x0A: 	printf(INDEX "\tTLTI\n",x); 	return;
		case 0x0B: 	printf(INDEX "\tTLTIU\n",x); 	return;
		case 0x0C: 	printf(INDEX "\tTEQI\n",x); 	return;
		case 0x0E: 	printf(INDEX "\tTNEI\n",x); 	return;
		case 0x10: 	printf(INDEX "\tBLTZAL \t" BV_OP "\n", x, OP_BV(uiMIPSword)); return;	// I and link
		case 0x11: 	printf(INDEX "\tBGEZAL \t" BV_OP "\n", x, OP_BV(uiMIPSword)); return;	// I and link
		case 0x12: 	printf(INDEX "\tBLTZALL\t" BV_OP "\n", x, OP_BV(uiMIPSword)); return;	// I and link likely
		case 0x13: 	printf(INDEX "\tBGEZALL\t" BV_OP "\n", x, OP_BV(uiMIPSword)); return;	// I and link likely
		}

		break;

	case 0x02: 	printf(INDEX "\tJ      \t" J_OP "\n", x, OP_J(uiMIPSword)); return;	// J
	case 0x03: 	printf(INDEX "\tJAL    \t" J_OP "\n", x, OP_J(uiMIPSword)); return;	// J
	case 0x04: 	printf(INDEX "\tBEQ    \t" B_OP "\n", x, OP_B(uiMIPSword)); return;	// I
	case 0x05: 	printf(INDEX "\tBNE    \t" B_OP "\n", x, OP_B(uiMIPSword)); return;	// I
	case 0x06: 	printf(INDEX "\tBLEZ   \t" BV_OP "\n", x, OP_BV(uiMIPSword)); return;	// I
	case 0x07: 	printf(INDEX "\tBGTZ   \t" BV_OP "\n", x, OP_BV(uiMIPSword)); return;	// I
	case 0x08: 	printf(INDEX "\tADDI   \t" I_OP "\n", x, OP_I(uiMIPSword)); return;	// I
	case 0x09: 	printf(INDEX "\tADDIU  \t" I_OP "\n", x, OP_I(uiMIPSword)); return;	// I
	case 0x0A: 	printf(INDEX "\tSLTI   \t" I_OP "\n", x, OP_I(uiMIPSword)); return;	// I
	case 0x0B: 	printf(INDEX "\tSLTIU  \t" I_OP "\n", x, OP_I(uiMIPSword)); return;	// I
	case 0x0C: 	printf(INDEX "\tANDI   \t" I_OP "\n", x, OP_I(uiMIPSword)); return;	// I
	case 0x0D: 	printf(INDEX "\tORI    \t" I_OP "\n", x, OP_I(uiMIPSword)); return;	// I
	case 0x0E: 	printf(INDEX "\tXORI   \t" I_OP "\n", x, OP_I(uiMIPSword)); return;	// I
	case 0x0F: 	printf(INDEX "\tLUI    \t" I_OP "\t Load upper half of word\n", x, OP_I(uiMIPSword)); return;	// I
	case 0x10: 	//printf(INDEX "\tcop0\n",x);
		op2=(uiMIPSword>>21)&0x1f;
		switch(op2)
		{
		case 0x00: printf(INDEX "\tMFC0 \t r%d, f%d \t// move f%d to r%d\traw 0x%x\n",x, (uiMIPSword>>16), (uiMIPSword>>11), (uiMIPSword>>16), (uiMIPSword>>11), uiMIPSword); return;
		case 0x04: printf(INDEX "\tMTC0 \t r%d, f%d \t// move r%d to f%d\traw 0x%x\n",x, (uiMIPSword>>16), (uiMIPSword>>11), (uiMIPSword>>16), (uiMIPSword>>11), uiMIPSword); return;
		case 0x10: printf(INDEX "\ttlb\n",x);
			switch(uiMIPSword&0x3f)
			{
			case 0x01: printf(INDEX "\tTLBR\n",x); return;
			case 0x02: printf(INDEX "\tTLBWI\n",x); return;
			case 0x06: printf(INDEX "\tTLBWR\n",x); return;
			case 0x08: printf(INDEX "\tTLBP\n",x); return;
			case 0x18: printf(INDEX "\tERET\n",x); return;
			}
		}
		break;

	case 0x11: //printf(INDEX "\tcop1\n",x);
		op2=(uiMIPSword>>21)&0x1f;
		switch(op2)
		{
		case 0x00: printf(INDEX "\tMFC1 \t r%d, f%d \t// move f%d to r%d\traw 0x%x\n",x, (uiMIPSword>>16), (uiMIPSword>>11), (uiMIPSword>>16), (uiMIPSword>>11), uiMIPSword); return;
		case 0x01: printf(INDEX "\tDMFC1\t r%d, f%d \t// move r%d to f%d\traw 0x%x\n",x, (uiMIPSword>>16), (uiMIPSword>>11), (uiMIPSword>>16), (uiMIPSword>>11), uiMIPSword); return;
		case 0x02: printf(INDEX "\tCFC1\n",x); return;
		case 0x04: printf(INDEX "\tMTC1\n",x); return;
		case 0x05: printf(INDEX "\tDMTC1\n",x); return;
		case 0x06: printf(INDEX "\tCTC1\n",x); return;
		case 0x08: printf(INDEX "\tBC1\n",x);
			switch((uiMIPSword>>16)&0x3)
			{
			case 0x00: printf(INDEX "\tBC1F",x); return;
			case 0x01: printf(INDEX "\tBC1T",x); return;
			case 0x02: printf(INDEX "\tBC1FL",x); return;
			case 0x03: printf(INDEX "\tBC1TL",x); return;
			}break;

		case 0x10: //printf(INDEX "\tC1.S\n",x);
			switch(uiMIPSword&0x3f)
			{
			case 0x00: printf(INDEX "\tADD.S\n",x); return;
			case 0x01: printf(INDEX "\tSUB.S\n",x); return;
			case 0x02: printf(INDEX "\tMUL.S\n",x); return;
			case 0x03: printf(INDEX "\tDIV.S\n",x); return;
			case 0x04: printf(INDEX "\tSQRT.S\n",x); return;
			case 0x05: printf(INDEX "\tABS.S\n",x); return;
			case 0x06: printf(INDEX "\tMOV.S\n",x); return;
			case 0x07: printf(INDEX "\tNEG.S\n",x); return;
			case 0x08: printf(INDEX "\tROUND.L.S\n",x); return;
			case 0x09: printf(INDEX "\tTRUNC.L.S\n",x); return;
			case 0x0A: printf(INDEX "\tCEIL.L.S\n",x); return;
			case 0x0B: printf(INDEX "\tFLOOR.L.S\n",x); return;
			case 0x0C: printf(INDEX "\tROUND.W.S\n",x); return;
			case 0x0D: printf(INDEX "\tTRUNC.W.S\n",x); return;
			case 0x0E: printf(INDEX "\tCEIL.W.S\n",x); return;
			case 0x0F: printf(INDEX "\tFLOOR.W.S\n",x); return;
			case 0x21: printf(INDEX "\tCVT.D.S\n",x); return;
			case 0x24: printf(INDEX "\tCVT.W.S\n",x); return;
			case 0x25: printf(INDEX "\tCVT.L.S\n",x); return;
			case 0x30: printf(INDEX "\tC.F.S\n",x); return;
			case 0x31: printf(INDEX "\tC.UN.S\n",x); return;
			case 0x32: printf(INDEX "\tC.EQ.S\n",x); return;
			case 0x33: printf(INDEX "\tC.UEQ.S\n",x); return;
			case 0x34: printf(INDEX "\tC.OLT.S\n",x); return;
			case 0x35: printf(INDEX "\tC.ULT.S\n",x); return;
			case 0x36: printf(INDEX "\tC.OLE.S\n",x); return;
			case 0x37: printf(INDEX "\tC.ULE.S\n",x); return;
			case 0x38: printf(INDEX "\tC.SF.S\n",x); return;
			case 0x39: printf(INDEX "\tC.NGLE.S\n",x); return;
			case 0x3A: printf(INDEX "\tC.SEQ.S\n",x); return;
			case 0x3B: printf(INDEX "\tC.NGL.S\n",x); return;
			case 0x3C: printf(INDEX "\tC.LT.S\n",x); return;
			case 0x3D: printf(INDEX "\tC.NGE.S\n",x); return;
			case 0x3E: printf(INDEX "\tC.LE.S\n",x); return;
			case 0x3F: printf(INDEX "\tC.NGT.S\n",x); return;
			}
			break;
		case 0x11: //printf(INDEX "\tC1.D\n",x);
			switch(uiMIPSword&0x3f)
			{
			case 0x00: printf(INDEX "\tADD.D\n",x); return;
			case 0x01: printf(INDEX "\tSUB.D\n",x); return;
			case 0x02: printf(INDEX "\tMUL.D\n",x); return;
			case 0x03: printf(INDEX "\tDIV.D\n",x); return;
			case 0x04: printf(INDEX "\tSQRT.D\n",x); return;
			case 0x05: printf(INDEX "\tABS.D\n",x); return;
			case 0x06: printf(INDEX "\tMOV.D\n",x); return;
			case 0x07: printf(INDEX "\tNEG.D\n",x); return;
			case 0x08: printf(INDEX "\tROUND.L.D\n",x); return;
			case 0x09: printf(INDEX "\tTRUNC.L.D\n",x); return;
			case 0x0A: printf(INDEX "\tCEIL.L.D\n",x); return;
			case 0x0B: printf(INDEX "\tFLOOR.L.D\n",x); return;
			case 0x0C: printf(INDEX "\tROUND.W.D\n",x); return;
			case 0x0D: printf(INDEX "\tTRUNC.W.D\n",x); return;
			case 0x0E: printf(INDEX "\tCEIL.W.D\n",x); return;
			case 0x0F: printf(INDEX "\tFLOOR.W.D\n",x); return;
			case 0x20: printf(INDEX "\tCVT.S.D\n",x); return;
			case 0x24: printf(INDEX "\tCVT.W.D\n",x); return;
			case 0x25: printf(INDEX "\tCVT.L.D\n",x); return;
			case 0x30: printf(INDEX "\tC.F.D\n",x); return;
			case 0x31: printf(INDEX "\tC.UN.D\n",x); return;
			case 0x32: printf(INDEX "\tC.EQ.D\n",x); return;
			case 0x33: printf(INDEX "\tC.UEQ.D\n",x); return;
			case 0x34: printf(INDEX "\tC.OLT.D\n",x); return;
			case 0x35: printf(INDEX "\tC.ULT.D\n",x); return;
			case 0x36: printf(INDEX "\tC.OLE.D\n",x); return;
			case 0x37: printf(INDEX "\tC.ULE.D\n",x); return;
			case 0x38: printf(INDEX "\tC.SF.D\n",x); return;
			case 0x39: printf(INDEX "\tC.NGLE.D\n",x); return;
			case 0x3A: printf(INDEX "\tC.SEQ.D\n",x); return;
			case 0x3B: printf(INDEX "\tC.NGL.D\n",x); return;
			case 0x3C: printf(INDEX "\tC.LT.D\n",x); return;
			case 0x3D: printf(INDEX "\tC.NGE.D\n",x); return;
			case 0x3E: printf(INDEX "\tC.LE.D\n",x); return;
			case 0x3F: printf(INDEX "\tC.NGT.D\n",x); return;
			}
			break;
		case 0x14: //printf(INDEX "\tC1.W\n",x);
			switch(uiMIPSword&0x3f)
			{
			case 0x20: printf(INDEX "\tCVT.S.W\n",x); return;
			case 0x21: printf(INDEX "\tCVT.D.W\n",x); return;
			}
			break;

		case 0x15: //printf(INDEX "\tC1.L\n",x);
			switch(uiMIPSword&0x3f)
			{
			case 0x20: printf(INDEX "\tCVT.S.L\n",x); return;
			case 0x21: printf(INDEX "\tCVT.D.L\n",x); return;
			}
			break;
		}
		break;

	case 0x14: printf(INDEX "\tBEQL  \n",x); return;
	case 0x15: printf(INDEX "\tBNEL  \n",x); return;
	case 0x16: printf(INDEX "\tBLEZL \n",x); return;
	case 0x17: printf(INDEX "\tBGTZL \n",x); return;
	case 0x18: printf(INDEX "\tDADDI \n",x); return;
	case 0x19: printf(INDEX "\tDADDIU\n",x); return;
	case 0x1A: printf(INDEX "\tLDL   \n",x); return;
	case 0x1B: printf(INDEX "\tLDR   \n",x); return;
	case 0x20: printf(INDEX "\tLB    \t" L_OP "\n", x, OP_L(uiMIPSword)); return;	// Load Byte
	case 0x21: printf(INDEX "\tLH    \t" L_OP "\n", x, OP_L(uiMIPSword)); return;	// Load Halfword
	case 0x22: printf(INDEX "\tLWL   \n",x); return;
	case 0x23: printf(INDEX "\tLW    \t" L_OP "\n", x, OP_L(uiMIPSword)); return;	// Load Word
	case 0x24: printf(INDEX "\tLBU   \t" L_OP "\n", x, OP_L(uiMIPSword)); return;	// Load Unsigned Byte
	case 0x25: printf(INDEX "\tLHU   \t" L_OP "\n", x, OP_L(uiMIPSword)); return;	// Load Halfword unsigned
	case 0x26: printf(INDEX "\tLWR   \n",x); return;
	case 0x27: printf(INDEX "\tLWU   \t" L_OP "\n", x, OP_L(uiMIPSword)); return;	// Load Word unsigned
	case 0x28: printf(INDEX "\tSB    \t" I_OP "\n", x, OP_I(uiMIPSword)); return;	// I
	case 0x29: printf(INDEX "\tSH    \t" I_OP "\n", x, OP_I(uiMIPSword)); return;	// I
	case 0x2A: printf(INDEX "\tSWL   \n",x); return;
	case 0x2B: printf(INDEX "\tSW    \t" I_OP "\n", x, OP_I(uiMIPSword)); return;	// I
	case 0x2C: printf(INDEX "\tSDL   \n",x); return;
	case 0x2D: printf(INDEX "\tSDR   \n",x); return;
	case 0x2E: printf(INDEX "\tSWR   \n",x); return;
	case 0x2F: printf(INDEX "\tCACHE \n",x); return;
	case 0x30: printf(INDEX "\tLL    \t" L_OP "\n", x, OP_L(uiMIPSword)); return;	// Load Linked Word atomic Read-Modify-Write ops
	case 0x31: printf(INDEX "\tLWC1  \t" L_OP "\n", x, OP_L(uiMIPSword)); return;	// Load Word to co processor 1
	case 0x34: printf(INDEX "\tLLD   \t" L_OP "\n", x, OP_L(uiMIPSword)); return;	// Load Linked Dbl Word atomic Read-Modify-Write ops
	case 0x35: printf(INDEX "\tLDC1  \n",x); return;
	case 0x37: printf(INDEX "\tLD    \t" L_OP "\n", x, OP_L(uiMIPSword)); return; 	// Load Double word
	case 0x38: printf(INDEX "\tSC    \t" L_OP "\n", x, OP_L(uiMIPSword)); return;	// Store Linked Word atomic Read-Modify-Write ops
	case 0x39: printf(INDEX "\tSWC1  \t" L_OP "\n", x, OP_L(uiMIPSword)); return;	// Store Word from co processor 1 to memory
	case 0x3C: printf(INDEX "\tSCD   \t" L_OP "\n", x, OP_L(uiMIPSword)); return;	// Store Conditional Double Word
	case 0x3D: printf(INDEX "\tSDC1  \n",x); return;
	case 0x3F: printf(INDEX "\tSD    \t" L_OP "\n", x, OP_L(uiMIPSword)); return; 	// Store Double word
	}

	printf(INDEX "\t...\t0x%08X\n",x, uiMIPSword); return;
}

