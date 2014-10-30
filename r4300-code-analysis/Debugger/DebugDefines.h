/*
 * DebugDefines.h
 *
 *  Created on: 23 Oct 2014
 *      Author: rjhender
 */

#ifndef DEBUGDEFINES_H_
#define DEBUGDEFINES_H_


// ========= TEST Setups ==============================================

//#define TEST_BRANCHING_FORWARD
//#define TEST_BRANCHING_BACKWARD
//#define TEST_BRANCH_TO_C
//#define TEST_LITERAL

// ========= Extra Debugging Information ==============================

// Print the register map when registers re-assigned (TranslateRegiters())
//#define SHOW_REG_TRANSLATION_MAP

// Print the constants in intermediate code
//#define SHOW_PRINT_INT_CONST

//Print the raw Hex values when reading arm instructions (arm_decode)
#define SHOW_PRINT_ARM_VALUE

//Print when a segment is deleted
#define SHOW_PRINT_SEGMENT_DELETE

// Print MIPS code / Translation step for each intermediate instruction
#define USE_INSTRUCTION_COMMENTS

//store the initial MIPS registers assigned to intermediate instruction
#define USE_INSTRUCTION_INIT_REGS

// Use the translation that adds debugging markers
#define USE_TRANSLATE_DEBUG

// ========= Recompiling Checks =======================================

// You can strip the REG_HOST flags when emiting arm code
// or when registers are translated.
#undef DO_HOSTREG_RENUMBER_IN_TRANSLATIONS


// ========= Customize Aborts for debugging ===========================

#define ABORT_ARM_DECODE
//#define ABORT_ARM_ENCODE
#define ABORT_EXCEEDED_GLOBAL_OFFSET


#endif /* DEBUGDEFINES_H_ */