#ifndef __LIPS_AST_H__
#define __LIPS_AST_H__

#include <lips/bytecode.h>
#include <notstd/utf8.h>

typedef enum{
	NOP_NEW,
	NOP_PARENT,
	NOP_DISABLE,
	NOP_ENABLE
}nodeOP_e;

typedef struct lipsAsl{
	nodeOP_e      op;
	uint16_t      id;
	const utf8_t* sp;
}lipsAsl_s;

typedef struct lipsAst{
	struct lipsAst* parent;
	struct lipsAst* child;
	const utf8_t*   sp;
	unsigned        len;
	uint16_t        id;
}lipsAst_s;

lipsAst_s* lips_ast_make(lipsAsl_s* node);

#endif

