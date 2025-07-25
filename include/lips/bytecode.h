#ifndef __LIPS_BYTECODE_H__
#define __LIPS_BYTECODE_H__

#include <notstd/core.h>

#define BYTECODE_FORMAT      0xEE1A
#define BYC_FORMAT           0x00
#define BYC_FLAGS            0x01
#define BYC_RANGE_COUNT      0x02
#define BYC_URANGE_COUNT     0x03
#define BYC_FN_COUNT         0x04
#define BYC_ERR_COUNT        0x05
#define BYC_SEMANTIC_COUNT   0x06
#define BYC_START            0x07
#define BYC_CODELEN          0x08
#define BYC_SECTION_FN       0x09
#define BYC_SECTION_NAME     0x0A
#define BYC_SECTION_ERROR    0x0B
#define BYC_SECTION_RANGE    0x0C
#define BYC_SECTION_URANGE   0x0D
#define BYC_SECTION_SEMANTIC 0x0E
#define BYC_SECTION_CODE     0x0F
#define BYC_HEADER_SIZE      0x10

#define BYTECODE_CMD40(BYTECODE)  ((BYTECODE)&0xF000)
#define BYTECODE_CMD04(BYTECODE)  ((BYTECODE)&0x0F00)
#define BYTECODE_VAL12(BYTECODE)  ((BYTECODE)&0x0FFF)
#define BYTECODE_IVAL11(BYTECODE) ((int16_t)(BYTECODE & 0x0800 ? -((int16_t)((BYTECODE)&0x7FF)): (int16_t)((BYTECODE)&0x07FF)))
#define BYTECODE_VAL08(BYTECODE)  ((BYTECODE)&0x00FF)
#define BYTECODE_VAL44(BYTECODE)  ((BYTECODE)&0x0F00)

#define BYTECODE_FLAG_BEGIN     0x01
#define BYTECODE_FLAG_MULTILINE 0x02

#define LIPS_MAX_CAPTURE 128
#define LIPS_NODE_START  UINT16_MAX

typedef enum{
	OP_CHAR     = 0x1000,
	OP_RANGE    = 0x2000,
	OP_URANGE   = 0x3000,
	OP_PUSH     = 0x4000,
	OP_PUSHR    = 0x5000,
	OP_JMP      = 0x7000,
	OP_CALL     = 0x8000,
	OP_NODE     = 0xA000,
	OP_ENTER    = 0xB000,
	OP_TYPE     = 0xC000,
	OP_EXT      = 0xF000,
	OPE_MATCH          = 0x0000,
	OPE_SAVE           = 0x0100,
	OPE_RET            = 0x0200,
	OPE_NODEEX         = 0x0300,
	OPEV_NODEEX_NEW    = 0x0000,
	OPEV_NODEEX_PARENT = 0x0001,
	OPEV_NODEEX_DISABLE= 0x0002,
	OPEV_NODEEX_ENABLE = 0x0003,
	OPE_LEAVE          = 0x0400,
	OPE_VALUE          = 0x0500,
	OPEV_VALUE_SET     = 0x0001,
	OPEV_VALUE_TEST    = 0x0002,
	OPE_ERROR          = 0x0F00
}lipsOP_e;

typedef struct lipsSddr{
	uint16_t* addr;
}lipsSAddr_s;

typedef struct lipsSFase{
	uint16_t* addr;
}lipsSFase_s;

typedef struct lipsByc{
	uint16_t*    bytecode;
	uint16_t*    fn;
	uint16_t*    range;
	uint16_t*    urange;
	uint16_t*    code;
	const char** fnName;
	const char** errStr;
	lipsSFase_s* sfase;
	uint16_t     format;
	uint16_t     flags;
	uint16_t     rangeCount;
	uint16_t     urangeCount;
	uint16_t     fnCount;
	uint16_t     errCount;
	uint16_t     semanticCount;
	uint16_t     start;
	uint16_t     codeLen;
	uint16_t     sectionFn;
	uint16_t     sectionError;
	uint32_t     sectionName;
	uint16_t     sectionRange;
	uint16_t     sectionURange;
	uint16_t     sectionSemantic;
	uint16_t     sectionCode;
}lipsByc_s;

lipsByc_s* lipsByc_ctor(lipsByc_s* byc, uint16_t* bytecode);
void lipsByc_dtor(lipsByc_s* byc);
long lipsByc_name_toid(lipsByc_s* byc, const char* name);


#endif
