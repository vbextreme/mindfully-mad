#include <lips/lips.h>

#include "lips_hardcoded.h"

lips_s* lips_ctor_minimal(lips_s* l){
	l->bytecode = lips_builtin_grammar_make();
	lipsByc_ctor(&l->byc, l->bytecode);
	lips_vm_ctor(&l->vm, &l->byc);
	lips_match_ctor(&l->match);
	return l;
}

lips_s* lips_ctor(lips_s* l){
	l->bytecode = HARDCODED_BYTECODE;
	lipsByc_ctor(&l->byc, l->bytecode);
	lips_vm_ctor(&l->vm, &l->byc);
	lips_match_ctor(&l->match);
	return l;
}

void lips_dtor(lips_s* l){
	lips_vm_dtor(&l->vm);
	lipsByc_dtor(&l->byc);
	lips_match_dtor(&l->match);
	if( l->bytecode != HARDCODED_BYTECODE ) m_free(l->bytecode);
}

uint16_t* lips_regex_minimal(lips_s* l, const utf8_t* source){
	lips_vm_reset(&l->vm, &l->match, source);
	lips_vm_match(&l->vm);
	if( l->match.count < 1 ) return NULL;
	if( !l->match.ast.root ) return NULL;
	return lips_builtin_emitter(&l->byc, l->match.ast.root);
}

uint16_t* lips_regex_minimal_dbg(lips_s* l, const utf8_t* source){
	lips_vm_reset(&l->vm, &l->match, source);
	lips_vm_match_debug(&l->vm);
	if( l->match.count < 1 ) return NULL;
	if( !l->match.ast.root ) return NULL;
	return lips_builtin_emitter(&l->byc, l->match.ast.root);
}


