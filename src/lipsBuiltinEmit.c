#include <lips/bytecode.h>
#include <lips/ast.h>
#include <lips/ccompiler.h>
#include "lipsGrammarNameEnum.h"

typedef struct emit{
	lipsByc_s* byc;
	lcc_s      lc;
	lipsAst_s* ast;
	unsigned   save;
}emit_s;

typedef void(*emit_f)(emit_s*, lipsAst_s*);

typedef enum{ QT_NONE, QT_ZO, QT_ZM, QT_OM, QT_SPEC} quantifier_e;

__private lipsAst_s* get_token(emit_s* e, lipsAst_s* n, unsigned child, unsigned id){
	if( child < m_header(n->child)->len && n->child[child].id == id ){
		return &n->child[child];
	}
	die("emit: token %s not have child %s", e->byc->name[n->id], e->byc->name[id]);
}

__private unsigned get_token_unsigned(emit_s* e, lipsAst_s* n, unsigned child, unsigned id){
	n = get_token(e, n, child, id);
	return strtoul((const char*)n->tp, NULL, 0);
}

__private const utf8_t* get_token_string(emit_s* e, lipsAst_s* n, unsigned child, unsigned id, unsigned* len){
	n = get_token(e, n, child, id);
	*len = n->len;
	return n->tp;
	die("emit: token %s not have child %s", e->byc->name[n->id], e->byc->name[id]);
}

__private int test_token(lipsAst_s* n, unsigned* child, unsigned id){
	if( *child < m_header(n->child)->len && n->child[*child].id == id ){
		++(*child);
		return 1;
	}
	return 0;
}

__private void choose_token_emit(emit_s* e, lipsAst_s* n, emit_f fn, unsigned id){
	const unsigned count = m_header(n->child)->len;
	if( count > 1 ){
		unsigned lblend = lcc_label_new(&e->lc);
		unsigned lblnext = lcc_label_new(&e->lc);
		lcc_split(&e->lc, lblnext);
		fn(e, get_token(e, n, 0, id));
		for(unsigned i = 0; i < count-1; ++i){
			lcc_jmp(&e->lc, lblend);
			lcc_label(&e->lc, lblnext);
			lblnext = lcc_label_new(&e->lc);
			lcc_split(&e->lc, lblnext);
			fn(e, get_token(e, n, 0, id));
		}
		lcc_jmp(&e->lc, lblend);
		lcc_label(&e->lc, lblnext);
		fn(e, get_token(e, n, 0, id));
	}
	else{
		fn(e, get_token(e, n, 0, id));
	}
}

__private quantifier_e get_quantifier(emit_s* e, lipsAst_s* n, int* lazy, unsigned* m, unsigned* M){
	if( m_header(n->child)->len == 2 ){
		get_token(e, n, 1, LGRAM_lazy);
		*lazy = 1;
	}
	else if( m_header(n->child)->len != 1 ){
		die("emit: quantifier have too many child: %u", m_header(n->child)->len);
	}
	else{
		*lazy = 0;
	}
	n = &n->child[0];
	switch( n->id ){
		default              : die("emit: unknow rule %s as child of quantifier", e->byc->name[n->id]);
		case LGRAM_qtype: 
			switch( *n->tp ){
				default: die("emit: unknow quantifier type '%c'", *n->tp);
				case '?': return QT_ZO;
				case '*': return QT_ZM;
				case '+': return QT_OM;
			}
		break;
		case LGRAM_qspec:{
			unsigned next = 0;
			if( test_token(n, &next, LGRAM_lnum) ){
				*m = 0;
				*M = get_token_unsigned(e, &n->child[0], 0, LGRAM_num);
				return QT_SPEC;
			}
			lipsAst_s* num = get_token(e, n, 0, LGRAM_lnum);
			*m = get_token_unsigned(e, num, 0, LGRAM_num);
			next = 1;
			if( test_token(n, &next, LGRAM_rnum) ){
				*M = get_token_unsigned(e, &n->child[1], 0, LGRAM_num);
			}
			else{
				*M = *m;
			}
			return QT_SPEC;
		}break;
	}
	die("emit: internal error, this never append");
}

__private void quantifier_emit(emit_s* e, lipsAst_s* n, emit_f fn, unsigned id){
	int lazy;
	unsigned m;
	unsigned M;
	unsigned next = 0;
	quantifier_e qt = QT_NONE;
	if( test_token(n, &next, LGRAM_quantifier) ){
		qt = get_quantifier(e, &n->child[1], &lazy, &m, &M);
	}
	unsigned lbl;
	unsigned lend;
	switch( qt ){
		default: die("emit: quantifier type internal error, this never append");
		case QT_ZO:
			lbl = lcc_label_new(&e->lc);
			if( lazy ) 
				lcc_splir(&e->lc, lbl);
			else
				lcc_split(&e->lc, lbl);
			fn(e, get_token(e, n, 0, id));
			lcc_label(&e->lc, lbl);
		break;
		
		case QT_ZM:
			lend = lcc_label_new(&e->lc);
			lbl = lcc_label_new(&e->lc);
			lcc_label(&e->lc, lbl);
			if( lazy ) 
				lcc_splir(&e->lc, lbl);
			else
				lcc_split(&e->lc, lbl);
			fn(e, get_token(e, n, 0, id));
			lcc_jmp(&e->lc, lbl);
			lcc_label(&e->lc, lend);
		break;
		
		case QT_OM:
			lbl = lcc_label_new(&e->lc);
			lcc_label(&e->lc, lbl);
			fn(e, get_token(e, n, 0, id));
			if( lazy ) 
				lcc_split(&e->lc, lbl);
			else
				lcc_splir(&e->lc, lbl);
		break;
		
		case QT_SPEC:
			die("TODO quantifier specifier is not supported");
		break;
		
		case QT_NONE: fn(e, get_token(e, n, 0, id)); break;
	}
}

__private int test_token_find(lipsAst_s* n, unsigned id){
	mforeach(n->child, i){
		if( n->child[i].id == id ) return i+1;
	}
	return 0;
}

__private void emit_builtin_error(emit_s* e, lipsAst_s* n){
	unsigned num = get_token_unsigned(e, n, 0, LGRAM_num);
	unsigned len;
	const utf8_t* name = get_token_string(e, n, 1, LGRAM_quoted, &len);
	long r;
	if( (r=lcc_error_add(&e->lc, (const char*)name, len)) < 0 ) lcc_err_die(&e->lc);
	if( r != num ) die("emit: aspected error num %u but make %ld", num, r);
}

__private void emit_rule_def(emit_s* e, lipsAst_s* n){
	unsigned len;
	const utf8_t* name = get_token_string(e, n, 0, LGRAM_ruleName, &len);
	unsigned unstore = test_token_find(n, LGRAM_unstore);
	unsigned match   = test_token_find(n, LGRAM_match);
	unsigned ctx = unstore ? 0: 1;
	ctx |= match << 1;
	long binErr = 0;
	unsigned te = test_token_find(n, LGRAM_rule_binerr);
	if( te ) binErr = get_token_unsigned(e, &n->child[--te], 0, LGRAM_num);
	lcc_fn_prolog(&e->lc, (const char*)name, len, ctx&1, binErr);
	e->lc.fn[e->lc.currentFN].userctx = ctx;
}

__private void emit_rx_class(emit_s* e, lipsAst_s* n){
	lccrange_s r;
	lcc_range_ctor(&r);
	const unsigned count = m_header(n->child)->len;
	unsigned start = 0;
	unsigned rev = 0;
	if( test_token(n, &start, LGRAM_rx_class_rev) ) rev = 1;
	for( unsigned i = start; i < count; ++i ){
		lipsAst_s* range = get_token(e, n, i, LGRAM_rx_range);
		switch( m_header(range->child)->len ){
			case 1: lcc_range_set(&r, *range->child[0].tp); break;
			case 2: for( unsigned j = *range->child[0].tp; j <= *range->child[1].tp; ++j ) lcc_range_set(&r, j); break;
			default: die("emit: rx_range wrong child number:: 0 > %u < 3", m_header(range->child)->len);
		}
	}
	if( rev ) lcc_range_reverse(&r);
	long ir = lcc_range_add(&e->lc, &r);
	if( ir  < 0 ) lcc_err_die(&e->lc);
	lcc_range(&e->lc, ir);
}

__private void emit_rx_altern(emit_s* e, lipsAst_s* n);
__private void emit_rx_group(emit_s* e, lipsAst_s* n){
	unsigned unnamed = 0;
	unsigned start = 0;
	if( test_token(n, &start, LGRAM_rx_unnamed) ){
		unnamed = 1;
	}
	else{
		lcc_save(&e->lc, e->save++);
	}
	emit_rx_altern(e, get_token(e, n, start, LGRAM_rx_altern));
	if( !unnamed ) lcc_save(&e->lc, e->save++);
}

__private void emit_rx_primary(emit_s* e, lipsAst_s* n){
	n = &n->child[0];
	switch( n->id ){
		default              : die("emit: unknow rule %s as child of rx_primary", e->byc->name[n->id]);
		case LGRAM_rx_literal: lcc_char(&e->lc, *n->tp); break;
		case LGRAM_rx_escaped: lcc_char(&e->lc, *n->tp); break;
		case LGRAM_rx_any    : lcc_range(&e->lc, 0); break;
		case LGRAM_rx_class  : emit_rx_class(e, n); break;
		case LGRAM_rx_group  : emit_rx_group(e, n); break;
	}
}

__private void emit_rx_repeat(emit_s* e, lipsAst_s* n){
	quantifier_emit(e, n, emit_rx_primary, LGRAM_rx_primary);
}

__private void emit_rx_concat(emit_s* e, lipsAst_s* n){
	mforeach(n->child, i){
		emit_rx_repeat(e, get_token(e, n, i, LGRAM_rx_repeat));
	}
}

__private void emit_rx_altern(emit_s* e, lipsAst_s* n){
	choose_token_emit(e, n, emit_rx_concat, LGRAM_rx_concat);
}

__private void emit_regex(emit_s* e, lipsAst_s* n){
	emit_rx_altern(e, get_token(e, n, 0, LGRAM_rx_altern));
}

__private void emit_rule_altern(emit_s* e, lipsAst_s* n);
__private void emit_rule_primary(emit_s* e, lipsAst_s* n){
	n = &n->child[0];
	switch( n->id ){
		case LGRAM_regex      : emit_regex(e, n); break;
		case LGRAM_rule_binerr: lcc_error(&e->lc, get_token_unsigned(e, n, 0, LGRAM_ErrNum)); break;
		case LGRAM_rule_group : emit_rule_altern(e, n); break;
		case LGRAM_call       : lcc_call(&e->lc, (const char*)n->tp, n->len); break;
		default: die("emit: unknow rule %s as child of rule_primary", e->byc->name[n->id]);
	}
}

__private void emit_rule_repeat(emit_s* e, lipsAst_s* n){
	quantifier_emit(e, n, emit_rule_primary, LGRAM_rule_primary);
}

__private void emit_rule_concat(emit_s* e, lipsAst_s* n){
	mforeach(n->child, i){
		emit_rule_repeat(e, get_token(e, n, i, LGRAM_rule_repeat));
	}
}

__private void emit_rule_altern(emit_s* e, lipsAst_s* n){
	choose_token_emit(e, n, emit_rule_concat, LGRAM_rule_concat);
}

__private void emit_rule(emit_s* e, lipsAst_s* n){
	emit_rule_def(e, get_token(e, n, 0, LGRAM_rule_def));
	emit_rule_def(e, get_token(e, n, 1, LGRAM_rule_altern));
	unsigned flags = e->lc.fn[e->lc.currentFN].userctx;
   	if( flags & 0x2 ) lcc_match(&e->lc, 1, 0);
	lcc_fn_epilog(&e->lc, flags & 1);
}

__private void emit_lips(emit_s* e, lipsAst_s* n){
	mforeach( n->child, j ){
		switch( n->child[j].id ){
			case LGRAM_builtin_error: emit_builtin_error(e, &n->child[j]); break;
			case LGRAM_rule         : emit_rule(e, &n->child[j]); break;
			case LGRAM_semantic:

			break;
			
			default: die("emit: unknow rule %s in as child of lips", e->byc->name[n->child[j].id]);
		}
	}
}

__private void emit_grammar(emit_s* e, lipsAst_s* n){
	mforeach(n->child, i){
		iassert(n->child[i].id == LGRAM_grammar);
		lipsAst_s* g = &n->child[i];
		mforeach( g->child, j ){
			switch( g->child[j].id ){
				case LGRAM_lips:
					emit_lips(e, &g->child[j]);
				break;
				default: die("emit: unknow rule %s in as child of grammar", e->byc->name[g->child[j].id]);
			}
		}
	}
}

uint16_t* lips_builtin_emitter(lipsByc_s* byc, lipsAst_s* ast){
	emit_s emit;
	emit.byc = byc;
	emit.ast = ast;
	emit.save = 2;
	lcc_ctor(&emit.lc);
	emit_grammar(&emit, emit.ast);
	//uint16_t* ret = lcc_make(&emit.lc);
	//if( !ret ) lcc_err_die(&emit.lc);
	//lcc_dtor(&emit.lc);
	//return ret;
	return NULL;
}
