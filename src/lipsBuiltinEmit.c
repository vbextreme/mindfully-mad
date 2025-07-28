#include <lips/bytecode.h>
#include <lips/ast.h>
#include <lips/ccompiler.h>
#include "lipsGrammarNameEnum.h"

typedef struct emit{
	lipsByc_s* byc;
	lcc_s      lc;
	lipsAst_s* ast;
}emit_s;


__private void emit_regex(emit_s* e, lipsAst_s* n){

}



__private void emit_builtin_error(emit_s* e, lipsAst_s* n){
	
}

__private void emit_lips(emit_s* e, lipsAst_s* n){
	mforeach( n->child, j ){
		switch( n->child[j].id ){
			case LGRAM_builtin_error:
				emit_builtin_error(e, &n->child[j]);
			break;
				
			case LGRAM_rule:
			break;
			
			case LGRAM_semantic:
			break;
			
			default: die("unknow rule %s in as child of lips", e->byc->name[n->child[j].id]);
		}
	}
}

__private void emit_grammar(emit_s* e, lipsAst_s* n){
	mforeach(n->child, i){
		iassert(n->child[i].id == LGRAM_grammar);
		lipsAst_s* g = &n->child[i];
		mforeach( g->child, j ){
			switch( g->child[j].id ){
				case LGRAM_regex:
				break;
				
				case LGRAM_lips:
					emit_lips(e, &g->child[j]);
				break;
				
				default: die("unknow rule %s in as child of grammar", e->byc->name[g->child[j].id]);
			}
		}
	}
}

uint16_t* lips_builtin_emitter(lipsByc_s* byc, lipsAst_s* ast){
	emit_s emit;
	emit.byc = byc;
	emit.ast = ast;
	lcc_ctor(&emit.lc);
	emit_grammar(&emit, emit.ast);
	//uint16_t* ret = lcc_make(&emit.lc);
	//if( !ret ) lcc_err_die(&emit.lc);
	//lcc_dtor(&emit.lc);
	//return ret;
	return NULL;
}
