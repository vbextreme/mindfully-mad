#include <lips/bytecode.h>
#include <lips/ast.h>
#include <lips/ccompiler.h>

typedef struct emit{
	lipsByc_s* byc;
	lcc_s      lc;
	lipsAst_s* ast;
	lipsAst_s* current;
}emit_s;

__private void emit_grammar(emit_s* e){
	
}

uint16_t* lips_builtin_emitter(lipsByc_s* byc, lipsAst_s* ast){
	emit_s emit;
	emit.byc = byc;
	emit.ast = ast;
	emit.current = ast;
	lcc_ctor(&emit.lc);
	
	uint16_t* ret = lcc_make(&emit.lc);
	if( !ret ) lcc_err_die(&emit.lc);
	lcc_dtor(&emit.lc);
	return ret;
}
