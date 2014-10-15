/*
 * CodeSegments.c
 *
 *  Created on: 16 Apr 2014
 *      Author: rjhender
 */

#include "CodeSegments.h"
#include "InstructionSetMIPS4.h"
#include "InstructionSetARM6hf.h"
#include "InstructionSet.h"
#include "Translate.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "memory.h"

//-------------------------------------------------------------------

code_segment_data_t segmentData;
//static uint8_t* CodeSegBounds;
static uint32_t GlobalLiteralCount = 0;

//==================================================================

#if 0
static uint32_t CountRegisers(uint32_t *bitfields)
{
	int x, y;
	uint32_t c = 0;

	for (y=0; y < 3; y ++)
	{
		for (x=0; x < 32; x++)
		{
			if (bitfields[y] & (1<<x)) c++;
		}
	}
	return c;
}
#endif

/*
 * Function to overwitre the Branch Statement at the end of ARM code so that it points to
 * 	code generated by Generate_BranchStubCode()
 */
static void invalidateBranch(code_seg_t* codeSegment)
{

}

//================== Searching ========================================

#if 0
static code_seg_t* Search_MIPS(uint32_t address)
{
	code_seg_t* seg;

	switch ((((uint32_t)address)>>23) & 0xFF)
		{
		case 0x80:
		case 0xA0:
			seg = segmentData.DynamicSegments;
			break;
		case 0x88:
			seg = segmentData.StaticSegments;
			break;
		case 0x90:
			printf("PIF boot ROM: ScanForCode()\n");
			return 0;
		default:
			break;
		}


	while (seg)
	{
		if (address == (uint32_t)seg->MIPScode) return seg;

		seg = seg->next;
	}
	return NULL;
}
#endif

//================== Literals ========================================

static literal_t* newLiteral(const uint32_t value)
{
	literal_t* newLiteral = malloc(sizeof(literal_t));

	newLiteral->value = value;
	newLiteral->next = NULL;

	return newLiteral;
}

static void freeLiterals(code_seg_t* codeSegment)
{
	literal_t *prev;
	literal_t *next;

	//remove any existing literals
	if (codeSegment->literals)
	{
		prev = codeSegment->literals;

		while (prev)
		{
			next = prev->next;
			free(prev);
			prev = next;
		}
	}
	codeSegment->literals = NULL;
}

// TODO what if segment length means offset > 4096?
uint32_t addLiteral(code_seg_t* const codeSegment, regID_t* const base, int32_t* const offset, const uint32_t value)
{
	int index = 4;

	if (SEG_SANDWICH == codeSegment->Type)
	{
		int x;
		for (x=1; x < 1024; x++)		//Scan existing global literals for Value
		{
			if (*((uint32_t*)MMAP_FP_BASE - x) == value)
			{
				*offset = x * 4;
				*base = REG_HOST_FP;
				return 0;
			}
		}

		if (GlobalLiteralCount >= 1024)
		{
			printf("CodeSegments.c:%d Run out of Global Literal positions\n", __LINE__);
#if defined(ABORT_EXCEEDED_GLOBAL_OFFSET)
			abort();
#endif
		}

		*offset = -(GlobalLiteralCount+1)*4;
		*base = REG_HOST_FP;
		*((uint32_t*)MMAP_FP_BASE - GlobalLiteralCount-1) = value;
		GlobalLiteralCount++;
		return 0;
	}

	if (codeSegment->literals)
	{
		//TODO if the value is already added then could re-use it

		literal_t* nxt = codeSegment->literals;
		while (nxt->next)
		{
			index+=4;
			nxt = nxt->next;
		}
		nxt->next = newLiteral(value);
		*offset = index;
		*base = REG_HOST_PC;
	}
	else
	{
		codeSegment->literals = newLiteral(value);
		*offset = 0;
		*base = REG_HOST_PC;
	}

	return 0;
}

//=================== Callers ========================================

static caller_t* newCaller(const code_seg_t* const caller)
{
	caller_t *newCaller;
	newCaller = malloc(sizeof(caller_t));

	newCaller->codeSeg = (code_seg_t*)caller;

	return newCaller;
}

static void updateCallers(code_seg_t* const codeSegment)
{
	caller_t *caller;

	if (codeSegment->callers)
	{
		caller = codeSegment->callers;

		while (caller)
		{
			invalidateBranch(caller->codeSeg);
			caller = caller->next;
		}
	}

}

static void freeCallers(code_seg_t* const codeSegment)
{
	caller_t *prev;
	caller_t *next;

	//remove any existing callers
	if (codeSegment->callers)
	{
		prev = codeSegment->callers;

		while (prev)
		{
			next = prev->next;
			free(prev);
			prev = next;
		}
	}
	codeSegment->callers = NULL;
}

static void addToCallers(const code_seg_t* const caller, code_seg_t* const callee)
{
	caller_t *currentCaller;
	if (!callee) return;

	//remove any existing callers
	if (callee->callers)
	{
		currentCaller = callee->callers;


		while (currentCaller->next) currentCaller = currentCaller->next;

		currentCaller->next = newCaller(caller);
	}
	else
	{
		callee->callers = newCaller(caller);
	}
}

//=================== Intermediate Code ===============================

void freeIntermediateInstructions(code_seg_t* const codeSegment)
{
	Instruction_t *prevInstruction;
	Instruction_t *nextInstruction;

	//remove any existing Intermediate code
	if (codeSegment->Intermcode)
	{
		prevInstruction = codeSegment->Intermcode;

		while (prevInstruction)
		{
			nextInstruction = prevInstruction->nextInstruction;
			free(prevInstruction);
			prevInstruction = nextInstruction;
		}
	}
	codeSegment->Intermcode = NULL;

	freeLiterals(codeSegment);
}

//=================== Segment Linked List =============================

static void AddSegmentToLinkedList(code_seg_t* const newSeg)
{
	code_seg_t* seg;
	code_seg_t** pseg;

	static code_seg_t* lastStaticSeg = NULL;

	newSeg->next = NULL;

	//TODO dynamic once DMA is sorted
	seg = segmentData.StaticSegments;
	pseg = &segmentData.StaticSegments;

	if (seg == NULL)
	{
		*pseg = lastStaticSeg = newSeg;
	}
	else if (seg->next == NULL)
	{
		if ((*pseg)->MIPScode < newSeg->MIPScode)
		{
			(*pseg)->next = newSeg;
			newSeg->prev = *pseg;
			lastStaticSeg = newSeg;
		}else
		{
			newSeg->next = *pseg;
			(*pseg)->prev = newSeg;
			*pseg= newSeg;
		}
	}
	else
	{
		//fast forward to last segment
		if (lastStaticSeg->MIPScode < newSeg->MIPScode) seg = lastStaticSeg;

		//TODO could rewind from last ...
		while ((seg->next) && (seg->next->MIPScode < newSeg->MIPScode))
		{
			seg = seg->next;
		}

		if (NULL == seg->next) lastStaticSeg = newSeg;

		// seg->next will either be NULL or seg->next->MIPScode is greater than newSeg->MIPScode
		newSeg->next = seg->next;
		newSeg->prev = seg;

		if (seg->next) seg->next->prev = newSeg;
		seg->next = newSeg;
	}
}

static void RemoveSegmentFromLinkedList(code_seg_t* const codeSegment)
{
	if (codeSegment->prev) codeSegment->prev->next = codeSegment->next;
	if (codeSegment->next) codeSegment->next->prev = codeSegment->prev;
}

//================ Segment Generation/Linking =========================

code_seg_t* newSegment()
{
	code_seg_t* newSeg = malloc(sizeof(code_seg_t));
	memset(newSeg, 0,sizeof(code_seg_t));

	return newSeg;
}

uint32_t delSegment(code_seg_t* codeSegment)
{
	uint32_t ret = 0;

	freeIntermediateInstructions(codeSegment);	// free memory used for Intermediate Instructions
	freeLiterals(codeSegment);					// free memory used by literals

	updateCallers(codeSegment);					// Update all segments that branch to this one so that they point to stub
	freeCallers(codeSegment);					// free memory used by callers

	RemoveSegmentFromLinkedList(codeSegment);

	free(codeSegment);

	return ret;
}

/*
 * Generate Code block validity
 *
 * Scan memory for code segments.
 * */
static int32_t UpdateCodeBlockValidity(code_seg_t** const Block, const uint32_t* const address, const uint32_t length, const uint32_t upperAddress)
{
	code_seg_t* 	newSeg;

	int32_t 		x, y, z;
	uint32_t 		prevWordCode 	= 0;
	int32_t 		SegmentsCreated = 0;
	int32_t 		percentDone 	= 0;
	int32_t 		percentDone2 	= 0;
	int32_t 		countJumps 		= 0;
	Instruction_e 	op;

	for (x=0; x < length/4; x++)
	{
		percentDone = (400*x/length);

		if ((percentDone%5) == 0 && (percentDone != percentDone2))
		{
			printf("%2d%% done. x %d / %d, jumps %d\n", percentDone, x, length/4, countJumps);
			percentDone2 = percentDone;
		}

		op = ops_type(address[x]);

		if (INVALID == op) continue;

		for (y = x+1; y < length/4; y++)
		{
			op = ops_type(address[y]);

			//we are not in valid code
			if (INVALID == op)
			{
				prevWordCode = 0;
				break;
			}

			if ((op & OPS_JUMP) == OPS_JUMP)
			{
				uint32_t uiAddress = ops_JumpAddress(&address[y]);

				if (op & OPS_LINK)
				{
					if ((y+1 >= length/4) || INVALID == ops_type(address[y+1]))
					{
						prevWordCode = 0;
						break;
					}
				}
				if ( uiAddress >= upperAddress)	// bad offset
				{
					prevWordCode = 0;
					break;
				}

				countJumps++;
				newSeg = newSegment();
				newSeg->MIPScode = (uint32_t*)(address + x);
				newSeg->MIPScodeLen = y - x + 1;

				if (op == JR) //only JR can set PC to the Link Register (or other register!)
				{
					newSeg->MIPSReturnRegister = (*address>>21)&0x1f;
				}

				if (!prevWordCode)
					newSeg->Type = SEG_ALONE;
				else
					newSeg->Type = SEG_END;

				for (z = x; z < y + 1; z++)
				{
					Block[z] = newSeg;
				}

				SegmentsCreated++;
				AddSegmentToLinkedList(newSeg);

				if (op & OPS_LINK) prevWordCode = 1;
				else prevWordCode = 0;
				break;
			}
			else if((op & OPS_BRANCH) == OPS_BRANCH)	//MIPS does not have an unconditional branch
			{
				if ((y+1 >= length/4) || INVALID == ops_type(address[y+1]))
				{
					prevWordCode = 0;
					break;
				}

				int32_t offset =  ops_BranchOffset(&address[y]);

				if ((y + offset >= length/4) || INVALID == ops_type(address[y + offset]))
				{
					prevWordCode = 0;
					break;
				}

				countJumps++;
				newSeg = newSegment();

				//Is this an internal branch - need to create two segments
				// if use x<= y + offset then may throw SIGSEGV if offset is -1!
				if (offset < 0 && x < y + offset)
				{
					newSeg->MIPScode = (uint32_t*)(address + x);
					newSeg->MIPScodeLen = y - x + offset + 1;

					if (!prevWordCode)
						newSeg->Type = SEG_START;
					else
						newSeg->Type = SEG_SANDWICH;

					for (z = x; z < y + offset + 1; z++)
					{
						Block[z] = newSeg;
					}
					SegmentsCreated++;
					AddSegmentToLinkedList(newSeg);

					newSeg = newSegment();
					newSeg->MIPScode = (uint32_t*)(address + y + offset + 1);
					newSeg->MIPScodeLen = -offset;
					newSeg->Type = SEG_SANDWICH;

					for (z = y + offset + 1; z < y + 1; z++)
					{
						Block[z] = newSeg;
					}
					SegmentsCreated++;
					AddSegmentToLinkedList(newSeg);

				}
				else // if we are branching external to the block?
				{
					newSeg->MIPScode = (uint32_t*)(address + x);
					newSeg->MIPScodeLen = y - x + 1;

					if (!prevWordCode)
						newSeg->Type = SEG_START;
					else
						newSeg->Type = SEG_SANDWICH;

					for (z = x; z < y + 1; z++)
					{
						Block[z] = newSeg;
					}
					SegmentsCreated++;
					AddSegmentToLinkedList(newSeg);
				}

				prevWordCode = 1;
				break;
			}
		} // for (y = x; y < length/4; y++)
		x = y;
	} // for (x=0; x < length/4; x++)
	return SegmentsCreated;
}

/*
 * TODO check if linking to an instruction that is NOT the first in a segment
 */
static void LinkStaticSegments()
{
	code_seg_t* 	seg;
	code_seg_t* 	searchSeg;
	Instruction_e 	op;
	uint32_t* 		pMIPSinstuction;
	uint32_t 		uiCountSegsProcessed = 0;

	seg = segmentData.StaticSegments;

	while (seg)
	{
		segmentData.dbgCurrentSegment = seg;

		//if (uiCountSegsProcessed%2000 == 0) printf("linking segment %d (%d%%)\n", uiCountSegsProcessed, uiCountSegsProcessed*100/segmentData.count);
		uiCountSegsProcessed++;
		pMIPSinstuction = (seg->MIPScode + seg->MIPScodeLen - 1);
		
		op = ops_type(*pMIPSinstuction);
		//This segment could branch to itself or another

		if ((op & OPS_JUMP) == OPS_JUMP)
		{
			uint32_t uiAddress = ops_JumpAddress(pMIPSinstuction);

			if ((uint32_t)seg->MIPScode >= 0x881230e4 && (uint32_t)seg->MIPScode < 0x881232e4+100) printf("OPS_JUMP: insAddr 0x%X, Link %d, addr %u (0x%X)\n", (uint32_t)pMIPSinstuction, op & OPS_LINK? 1:0, uiAddress, uiAddress);

			//if (ops_type(*word) == JR) //only JR can set PC to the Link Register (or other register!)
					//(*(seg->MIPScode + seg->MIPScodeLen-1)>>21)&0x1f;
			searchSeg = (segmentData.StaticBounds[uiAddress/4]);

			if (searchSeg)
			{
				seg->pBranchNext = searchSeg;
				addToCallers(seg, searchSeg);
			}
		}
		//TODO use StaticBounds map to get segment for linking
		else if((op & OPS_BRANCH) == OPS_BRANCH)	//MIPS does not have an unconditional branch
		{
			int32_t offset =  ops_BranchOffset(pMIPSinstuction);

			searchSeg = (segmentData.StaticBounds[((uint32_t)seg->MIPScode - MMAP_STATIC_REGION)/4 + offset]);
			if (searchSeg)
			{
				seg->pBranchNext = searchSeg;
				addToCallers(seg, searchSeg);
			}

			/*if (-offset == seg->MIPScodeLen)
			{
				seg->pBranchNext = seg;
				addToCallers(seg, seg);
			}
			else if (offset < 0)
			{
				searchSeg = seg->prev;
				while (searchSeg)
				{
					if (pMIPSinstuction + offset == searchSeg->MIPScode)
					{
						seg->pBranchNext = searchSeg;
						addToCallers(seg, searchSeg);
						break;
					}

					if (pMIPSinstuction + offset < searchSeg->MIPScode) break;

					searchSeg = searchSeg->prev;
				}
			}
			else
			{
				searchSeg = seg->next;
				while (searchSeg)
				{
					if (pMIPSinstuction + offset == searchSeg->MIPScode)
					{
						seg->pBranchNext = searchSeg;
						addToCallers(seg, searchSeg);
						break;
					}

					if (pMIPSinstuction + offset < searchSeg->MIPScode) break;

					searchSeg = searchSeg->next;
				}
			}*/

			seg->pContinueNext = seg->next;
			addToCallers(seg, seg->next);
		}
		else // this must be a continue only segment
		{
			seg->pContinueNext = seg->next;
			addToCallers(seg, seg->next);
		}
		seg = seg->next;
	}
}

/*
 * Function to scan address range to find MIPS code.
 * The address could be an emulated virtual addresses
 *
 *  1. If the addres
 * 	1. Invalidate any code_segments that have changed
 * 	2. Generate new code_segments for address range
 *
 * 	Returns number of Segments Added (+)/Removed (-)
 *
 */
int32_t ScanForCode(const uint32_t* const address, const uint32_t length)
{
	uint32_t* addr = (uint32_t*)address;
	uint32_t upperAddress;
	code_seg_t** Bounds;

	switch ((((uint32_t)address)>>24) & 0xFF)
	{
	case 0x88:
		Bounds = &segmentData.StaticBounds[((uint32_t)address-0x88000000)/4];
		upperAddress = segmentData.StaticLength;
		addr = (uint32_t*)((uint32_t)address & 0x88FFFFFF);
		break;
	case 0x90:
		printf("PIF boot ROM: ScanForCode()\n");
		return 0;

	case 0x80:
	case 0xA0:
	default:
		Bounds = &segmentData.DynamicBounds[((uint32_t)address)/4];
		upperAddress = segmentData.DynamicLength;
		addr = (uint32_t*)((uint32_t)address & 0x80FFFFFF);
		break;
	}

	return UpdateCodeBlockValidity(Bounds, addr, length, upperAddress);
}

code_seg_t* getSegmentAt(void* address)
{
	if (address >= 0x88000000)
	{
		return segmentData.StaticBounds[((uint32_t)address - 0x88000000)/4];
	}
	else
	{
		return segmentData.DynamicBounds[((uint32_t)address)/4];
	}

}

/*
 * Function to Generate a code_segment_data structure
 *
 * It assumes memory has been mapped (at 0x80000000) and the ROM suitably copied into 0x88000000
 */
code_segment_data_t* GenerateCodeSegmentData(const int32_t ROMsize)
{
	segmentData.StaticSegments = NULL;
	segmentData.DynamicSegments = NULL;

	segmentData.StaticLength  = ROMsize*sizeof(*segmentData.StaticBounds)/4;
	segmentData.DynamicLength = RD_RAM_SIZE*sizeof(*segmentData.DynamicBounds)/4;

	segmentData.StaticBounds = malloc(segmentData.StaticLength);
	memset(segmentData.StaticBounds, 0, segmentData.StaticLength);
	printf("StaticBounds %d Bytes (0x%x), %d elements\n", segmentData.StaticLength, segmentData.StaticLength, segmentData.StaticLength/sizeof(*segmentData.StaticBounds));

	segmentData.DynamicBounds = malloc(segmentData.DynamicLength);
	memset(segmentData.DynamicBounds, 0, segmentData.DynamicLength);
	printf("DynamicBounds %d Bytes (0x%x), %d elements\n", segmentData.DynamicLength, segmentData.DynamicLength, segmentData.DynamicLength/sizeof(*segmentData.DynamicBounds));

	segmentData.count = ScanForCode((uint32_t*)(ROM_ADDRESS+64), ROMsize-64);

	printf("%d segments created\n", segmentData.count);

	LinkStaticSegments();

	// Build the helper functions

	segmentData.segStart = Generate_CodeStart(&segmentData);
	emit_arm_code(segmentData.segStart);
	*((uint32_t*)(MMAP_FP_BASE + FUNC_GEN_START)) = (uint32_t)segmentData.segStart->ARMEntryPoint;

	segmentData.segStop = Generate_CodeStop(&segmentData);
	emit_arm_code(segmentData.segStop);
	*((uint32_t*)(MMAP_FP_BASE + FUNC_GEN_STOP)) = (uint32_t)segmentData.segStop->ARMEntryPoint;
/*
	segmentData.segMem = Generate_MemoryTranslationCode(&segmentData, NULL);
	emit_arm_code(segmentData.segMem);
	*((uint32_t*)(MMAP_FP_BASE + FUNC_GEN_LOOKUP_VIRTUAL_ADDRESS)) = (uint32_t)segmentData.segMem->ARMEntryPoint;

	segmentData.segInterrupt = Generate_ISR(&segmentData);
	emit_arm_code(segmentData.segInterrupt);
	*((uint32_t*)(MMAP_FP_BASE + FUNC_GEN_INTERRUPT)) = (uint32_t)segmentData.segInterrupt->ARMEntryPoint;

	segmentData.segBranchUnknown = Generate_BranchUnknown(&segmentData);
	emit_arm_code(segmentData.segBranchUnknown);
	*((uint32_t*)(MMAP_FP_BASE + FUNC_GEN_BRANCH_UNKNOWN)) = (uint32_t)segmentData.segBranchUnknown->ARMEntryPoint;

	segmentData.segTrap = Generate_MIPS_Trap(&segmentData);
	emit_arm_code(segmentData.segTrap);
	*((uint32_t*)(MMAP_FP_BASE + FUNC_GEN_TRAP)) = (uint32_t)segmentData.segTrap->ARMEntryPoint;
*/
	// Compile the First contiguous block of Segments
	code_seg_t* seg = segmentData.StaticSegments;

	while (seg->pContinueNext)
	{
		segmentData.dbgCurrentSegment = seg;
		Translate(seg);

		emit_arm_code(seg);

		seg = seg->next;
	}
	segmentData.dbgCurrentSegment = seg;
	Translate(seg);

	*((uint32_t*)(MMAP_FP_BASE + RECOMPILED_CODE_START)) = (uint32_t)segmentData.StaticSegments->ARMEntryPoint;

#if 0
	printf("FUNC_GEN_START                   0x%x\n", (uint32_t)segmentData.segStart->ARMEntryPoint);
	printf("FUNC_GEN_STOP                    0x%x\n", (uint32_t)segmentData.segStop->ARMEntryPoint);
	printf("FUNC_GEN_LOOKUP_VIRTUAL_ADDRESS  0x%x\n", (uint32_t)segmentData.segMem->ARMEntryPoint);
	printf("FUNC_GEN_INTERRUPT               0x%x\n", (uint32_t)segmentData.segInterrupt->ARMEntryPoint);
	printf("FUNC_GEN_BRANCH_UNKNOWN          0x%x\n", (uint32_t)segmentData.segBranchUnknown->ARMEntryPoint);
	printf("FUNC_GEN_TRAP                    0x%x\n", (uint32_t)segmentData.segTrap->ARMEntryPoint);
	printf("RECOMPILED_CODE_START            0x%x\n", (uint32_t)segmentData.StaticSegments->ARMEntryPoint);

#endif

	segmentData.dbgCurrentSegment = segmentData.StaticSegments;


	return &segmentData;

}
