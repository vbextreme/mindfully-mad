#ifndef __MFM_REVM_H__
#define __MFM_REVM_H__

#include <notstd/core.h>
#include <notstd/utf8.h>

#define BYTECODE_FORMAT   0xEE1A
#define BYC_FORMAT        0
#define BYC_FLAGS         1
#define BYC_RANGE_COUNT   2
#define BYC_URANGE_COUNT  3
#define BYC_START         4
#define BYC_LEN           5
#define BYC_SECTION_FN    6
#define BYC_SECTION_RANGE 7

#define BYTECODE_CMD412(BYTECODE) ((BYTECODE)&0xF000)
#define BYTECODE_VAL412(BYTECODE) ((BYTECODE)&0x0FFF)
#define BYTECODE_VAL48(BYTECODE)  ((BYTECODE)&0x00FF)
#define BYTECODE_VAL44(BYTECODE)  ((BYTECODE)&0x0F00)

#define BYTECODE_FLAG_BEGIN     0x01
#define BYTECODE_FLAG_MULTILINE 0x02
#define BYTECODE_FLAG_GREEDY    0x03

#define REVM_MAX_CAPTURE 128

#define OP_SPLIT_ADD 0x0100
#define OP_SPLIT_SUB 0x0200


typedef enum{
	CMD_MATCH  = 0x0000,
	CMD_CHAR   = 0x1000,
	CMD_UNICODE= 0x2000,
	CMD_RANGE  = 0x3000,
	CMD_URANGE = 0x4000,
	CMD_SPLIT  = 0x5000,
	CMD_SPLIR  = 0x6000,
	CMD_JMP    = 0x7000,
	CMD_SAVE   = 0x8000,
	CMD_CALL   = 0x9000,
	CMD_RET    = 0xA000
}revmCmd_e;

typedef enum{
	OP_MATCH  = 0x0000,
	OP_CHAR   = 0x1000,
	OP_RANGE  = 0x2000,
	OP_URANGE = 0x3000,
	OP_SPLITF = 0x4000,
	OP_SPLITB = 0x5000,
	OP_SPLIRF = 0x6000,
	OP_SPLIRB = 0x7000,
	OP_JMPF   = 0x8000,
	OP_JMPB   = 0x9000,
	OP_SAVEB  = 0xA000,
	OP_SAVEE  = 0xB000,
	OP_CALL   = 0xD000,
	OP_RET    = 0xE000
}revmOP_e;



typedef struct revmMatch{
	const utf8_t* capture[REVM_MAX_CAPTURE * 2];
	int match;
}revmMatch_s;

revmMatch_s revm_match(const utf8_t* txt, const uint16_t* bytecode);



#endif

