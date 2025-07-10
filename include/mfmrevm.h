#ifndef __MFM_REVM_H__
#define __MFM_REVM_H__

#include <notstd/core.h>
#include <notstd/utf8.h>

#define BYTECODE_FORMAT   0xEE1A
#define BYC_FORMAT         0x0
#define BYC_FLAGS          0x1
#define BYC_RANGE_COUNT    0x2
#define BYC_URANGE_COUNT   0x3
#define BYC_FN_COUNT       0x4
#define BYC_START          0x5
#define BYC_CODELEN        0x6
#define BYC_SECTION_FN     0x7
#define BYC_SECTION_RANGE  0x8
#define BYC_SECTION_URANGE 0x9
#define BYC_SECTION_NAME   0xA
#define BYC_SECTION_CODE   0xB
#define BYC_HEADER_SIZE    0xC

#define BYTECODE_CMD40(BYTECODE) ((BYTECODE)&0xF000)
#define BYTECODE_CMD04(BYTECODE) ((BYTECODE)&0x0F00)
#define BYTECODE_VAL12(BYTECODE) ((BYTECODE)&0x0FFF)
#define BYTECODE_VAL08(BYTECODE)  ((BYTECODE)&0x00FF)
#define BYTECODE_VAL44(BYTECODE)  ((BYTECODE)&0x0F00)

#define BYTECODE_FLAG_BEGIN     0x01
#define BYTECODE_FLAG_MULTILINE 0x02

#define REVM_MAX_CAPTURE 128
#define REVM_NODE_START  UINT16_MAX

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
}revmOP_e;

typedef enum{
	NOP_NEW,
	NOP_PARENT,
	NOP_DISABLE,
	NOP_ENABLE
}nodeOP_e;

typedef struct bcnode{
	nodeOP_e      op;
	uint16_t      id;
	const utf8_t* sp;
}bcnode_s;

typedef struct reAst{
	struct reAst* parent;
	struct reAst* child;
	const utf8_t* sp;
	unsigned      len;
	uint16_t      id;
}reAst_s;

typedef struct revmMatch{
	reAst_s*      ast;
	const utf8_t* capture[REVM_MAX_CAPTURE * 2];
	int           match;
}revmMatch_s;

revmMatch_s revm_match(uint16_t* bytecode, const utf8_t* txt);
const char** revm_map_name(const uint16_t* bytecode);
void revm_debug(uint16_t* bytecode, const utf8_t* txt);

reAst_s* reast_make(bcnode_s* node);

#endif

