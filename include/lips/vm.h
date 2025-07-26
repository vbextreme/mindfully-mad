#ifndef __LIPS_VM_H__
#define __LIPS_VM_H__

#include <lips/bytecode.h>
#include <lips/ast.h>

typedef struct lipsError{
	const utf8_t* loc;
	uint8_t       number;
	lipsAst_s*    last;
	//lipsAsl_s*    asl;
}lipsError_s;

typedef struct lipsMatch{
	lipsError_s   err;
	lipsAst_s*    ast;
	const utf8_t* capture[LIPS_MAX_CAPTURE * 2];
	int           count;
}lipsMatch_s;

typedef struct lipsStack{
	const utf8_t* sp;
	lipsAst_s*    ip;
	unsigned      pc;
	int           ls;
	unsigned      cp;
	unsigned      np;
}lipsStack_s;

typedef struct lipsScope{
	struct lipsScope* child;
	struct lipsScope* parent;
	lipsAst_s**       symbols;
}lipsScope_s;

typedef struct lipsVM{
	lipsByc_s*     byc;
	lipsStack_s*   stack;
	lipsMatch_s*   match;
	unsigned*      cstk;
	lipsScope_s*   scope;
	lipsAsl_s*     node;
	lipsAst_s**    po;
	lipsAst_s*     ip;
	lipsScope_s*   sc;
	const utf8_t*  txt;
	const utf8_t*  sp;
	uint32_t       pc;
	uint8_t        er;
}lipsVM_s;

lipsVM_s* lips_vm_ctor(lipsVM_s* vm, lipsByc_s* byc);
void lips_vm_dtor(lipsVM_s* vm);
lipsMatch_s* lips_match_ctor(lipsMatch_s* match);
void lips_match_dtor(lipsMatch_s* match);
void lips_vm_reset(lipsVM_s* vm, lipsMatch_s* match, const utf8_t* txt);
void lips_vm_semantic_reset(lipsVM_s* vm);
int lips_vm_exec(lipsVM_s* vm);
int lips_vm_match(lipsVM_s* vm);
int lips_range_test(lipsVM_s* vm, unsigned ir, utf8_t ch );

#endif

