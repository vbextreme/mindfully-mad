#ifndef __LIPS_VM_H__
#define __LIPS_VM_H__

#include <lips/bytecode.h>
#include <lips/ast.h>

typedef struct lipsError{
	const utf8_t* loc;
	uint8_t       number;
}lipsError_s;

typedef struct lipsMatch{
	lipsError_s   err;
	lipsAst_s*    ast;
	const utf8_t* capture[LIPS_MAX_CAPTURE * 2];
	int           count;
}lipsMatch_s;

typedef struct lipsStack{
	unsigned      pc;
	const utf8_t* sp;
	int           ls;
	unsigned      cp;
	unsigned      np;
}lipsStack_s;

typedef struct lipsVM{
	lipsByc_s*     byc;
	lipsStack_s*   stack;
	lipsMatch_s*   match;
	unsigned*      cstk;
	lipsAsl_s*     node;
	const utf8_t*  sp;
	const utf8_t*  txt;
	uint32_t       pc;
	int            ls;
}lipsVM_s;

lipsVM_s* lips_vm_ctor(lipsVM_s* vm, lipsByc_s* byc);
void lips_vm_dtor(lipsVM_s* vm);
void lips_vm_reset(lipsVM_s* vm, lipsMatch_s* match, const utf8_t* txt);
int lips_vm_exec(lipsVM_s* vm);
int lips_vm_match(lipsVM_s* vm);

#endif

