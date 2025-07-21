#ifndef __LIPS_BYTECODE_H__
#define __LIPS_BYTECODE_H__

#include <notstd/core.h>

#define BYTECODE_FORMAT   0xEE1A
#define BYC_FORMAT         0x0
#define BYC_FLAGS          0x1
#define BYC_RANGE_COUNT    0x2
#define BYC_URANGE_COUNT   0x3
#define BYC_FN_COUNT       0x4
#define BYC_ERR_COUNT      0x5
#define BYC_START          0x6
#define BYC_CODELEN        0x7
#define BYC_SECTION_FN     0x8
#define BYC_SECTION_NAME   0x9
#define BYC_SECTION_ERROR  0xA
#define BYC_SECTION_RANGE  0xB
#define BYC_SECTION_URANGE 0xC
#define BYC_SECTION_CODE   0xD
#define BYC_HEADER_SIZE    0xE

#define BYTECODE_CMD40(BYTECODE) ((BYTECODE)&0xF000)
#define BYTECODE_CMD04(BYTECODE) ((BYTECODE)&0x0F00)
#define BYTECODE_VAL12(BYTECODE) ((BYTECODE)&0x0FFF)
#define BYTECODE_VAL08(BYTECODE)  ((BYTECODE)&0x00FF)
#define BYTECODE_VAL44(BYTECODE)  ((BYTECODE)&0x0F00)

#define BYTECODE_FLAG_BEGIN     0x01
#define BYTECODE_FLAG_MULTILINE 0x02

#define LIPS_MAX_CAPTURE 128
#define LIPS_NODE_START  UINT16_MAX

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
	OP_NODE   = 0xA000,
	OP_CALL   = 0xE000,
	OP_EXT    = 0xF000,
	OPE_SAVE   = 0x0000,
	OPE_RET    = 0x0100,
	OPE_NODEEX = 0x0200,
	OPE_ERROR  = 0x0F00
}lipsOP_e;

typedef struct lipsByc{
	uint16_t*    bytecode;
	uint16_t*    fn;
	uint16_t*    range;
	uint16_t*    urange;
	uint16_t*    code;
	const char** fnName;
	const char** errStr;
	uint16_t     format;
	uint16_t     flags;
	uint16_t     rangeCount;
	uint16_t     urangeCount;
	uint16_t     fnCount;
	uint16_t     errCount;
	uint16_t     start;
	uint16_t     codeLen;
	uint16_t     sectionFn;
	uint16_t     sectionError;
	uint32_t     sectionName;
	uint16_t     sectionRange;
	uint16_t     sectionURange;
	uint16_t     sectionCode;
}lipsByc_s;

lipsByc_s* lipsByc_ctor(lipsByc_s* byc, uint16_t* bytecode);
void lipsByc_dtor(lipsByc_s* byc);
long lipsByc_name_toid(lipsByc_s* byc, const char* name);


#endif
