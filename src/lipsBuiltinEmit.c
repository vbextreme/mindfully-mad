#include <lips/bytecode.h>
#include <lips/ast.h>
#include <lips/ccompiler.h>
#include "inutility.h"
#include "lipsGrammarNameEnum.h"

typedef struct emit{
	lipsByc_s* byc;
	lcc_s      lc;
	lipsAst_s* ast;
	unsigned   save;
	unsigned   isstart;
}emit_s;

typedef void(*emit_f)(emit_s*, lipsAst_s*);

typedef enum{ QT_NONE, QT_ZO, QT_ZM, QT_OM, QT_SPEC} quantifier_e;

__private lipsAst_s* get_token(lipsAst_s* n, unsigned child, unsigned id){
	if( child < m_header(n->child)->len && n->child[child]->id == id ){
		return n->child[child];
	}
	return NULL;
}

__private lipsAst_s* get_token_or_die(emit_s* e, lipsAst_s* n, unsigned child, unsigned id){
	lipsAst_s* ret = get_token(n, child, id);
	if( !ret ) die("emit: token %s not have child %s", e->byc->name[n->id], e->byc->name[id]);
	return ret;
}

__private unsigned get_token_unsigned_or_die(emit_s* e, lipsAst_s* n, unsigned child, unsigned id){
	n = get_token_or_die(e, n, child, id);
	return strtoul((const char*)n->tp, NULL, 0);
}

__private const utf8_t* get_token_string_or_die(emit_s* e, lipsAst_s* n, unsigned child, unsigned id, unsigned* len){
	n = get_token_or_die(e, n, child, id);
	*len = n->len;
	return n->tp;
}

__private void choose_token_emit(emit_s* e, lipsAst_s* n, emit_f fn, unsigned id){
	const unsigned count = m_header(n->child)->len;
	dbg_info("choose %u", count);
	if( count > 1 ){
		unsigned lblend = lcc_label_new(&e->lc);
		unsigned lblnext = lcc_label_new(&e->lc);
		lcc_split(&e->lc, lblnext);
		fn(e, get_token_or_die(e, n, 0, id));
		for(unsigned i = 1; i < count-1; ++i){
			lcc_jmp(&e->lc, lblend);
			lcc_label(&e->lc, lblnext);
			lblnext = lcc_label_new(&e->lc);
			lcc_split(&e->lc, lblnext);
			fn(e, get_token_or_die(e, n, i, id));
		}
		lcc_jmp(&e->lc, lblend);
		lcc_label(&e->lc, lblnext);
		fn(e, get_token_or_die(e, n, count-1, id));
		lcc_label(&e->lc, lblend);
	}
	else{
		fn(e, get_token_or_die(e, n, 0, id));
	}
}

__private quantifier_e get_quantifier(emit_s* e, lipsAst_s* n, int* lazy, unsigned* m, unsigned* M){
	dbg_info("");
	*lazy = 0;
	*m = 0;
	*M = 0;
	lipsAst_s* q = get_token(n, 1, LGRAM_quantifier);
	if( !q ) return QT_NONE;
	if( m_header(q->child)->len == 2 ){
		get_token_or_die(e, q, 1, LGRAM_lazy);
		*lazy = 1;
	}
	else if( m_header(q->child)->len != 1 ){
		die("emit: quantifier have too many child: %u", m_header(q->child)->len);
	}
	q = q->child[0];
	switch( q->id ){
		default              : die("emit: unknow rule %s as child of quantifier", e->byc->name[q->id]);
		case LGRAM_qtype: 
			switch( *q->tp ){
				default: die("emit: unknow quantifier type '%c'", *q->tp);
				case '?': return QT_ZO;
				case '*': return QT_ZM;
				case '+': return QT_OM;
			}
		break;
		case LGRAM_qspec:{
			unsigned next = 0;
			lipsAst_s* ln = get_token(q, next, LGRAM_lnum);
			if( ln ){
				*m = get_token_unsigned_or_die(e, ln, 0, LGRAM_num);
				*M = *m;
				++next;
			}
			lipsAst_s* rn = get_token(n, next, LGRAM_rnum);
			if( rn ){
				*M = get_token_unsigned_or_die(e, rn, 0, LGRAM_num);
				++next;
			}
			return QT_SPEC;
		}break;
	}
	die("emit: internal error, this never append");
}

__private void quantifier_emit(emit_s* e, lipsAst_s* n, emit_f fn, unsigned id){
	int lazy;
	unsigned m, M;
	quantifier_e qt = get_quantifier(e, n, &lazy, &m, &M);
	dbg_info("%c islazy:%u m:%u M:%u", 
		qt == QT_ZO ? '?' : qt == QT_ZM ? '*' : qt == QT_OM ? '+' : qt == QT_SPEC ? '{' : qt == QT_NONE ? '!' : 'E',
		lazy, m, M
	);

	unsigned L1, L2;
	switch( qt ){
		default: die("emit: quantifier type internal error, this never append");
		case QT_ZO:
			L2 = lcc_label_new(&e->lc);
			if( lazy ) 
				lcc_splir(&e->lc, L2);
			else
				lcc_split(&e->lc, L2);
			fn(e, get_token_or_die(e, n, 0, id));
			lcc_label(&e->lc, L2);
		break;
		
		case QT_ZM:
			L1 = lcc_label_new(&e->lc);
			L2 = lcc_label_new(&e->lc);
			lcc_label(&e->lc, L1);
			if( lazy ) 
				lcc_splir(&e->lc, L2);
			else
				lcc_split(&e->lc, L2);
			fn(e, get_token_or_die(e, n, 0, id));
			lcc_jmp(&e->lc, L1);
			lcc_label(&e->lc, L2);
		break;
		
		case QT_OM:
			L1 = lcc_label_new(&e->lc);
			lcc_label(&e->lc, L1);
			fn(e, get_token_or_die(e, n, 0, id));
			if( lazy ) 
				lcc_split(&e->lc, L1);
			else
				lcc_splir(&e->lc, L1);
		break;
		
		case QT_SPEC:
			die("TODO quantifier specifier is not supported");
		break;
		
		case QT_NONE: fn(e, get_token_or_die(e, n, 0, id)); break;
	}
}

__private unsigned unescape_char(lipsAst_s* n){
	n = n->child[0];
	unsigned r = 0;
	switch( n->id ){
		case LGRAM_rx_literal: r = *n->tp; break;
		case LGRAM_rx_escaped:
			iassert(n->len == 2);
			switch( n->tp[1] ){
				case 'n': r = '\n';     break;
				case 't': r = '\t';     break;
				case '0': r = 0;        break;
				default : r = n->tp[1]; break;
			}
		break;
		default: die("emit: rx_class wrong child, unaspected %u", n->id);
	}
	dbg_info("char '%s'", cast_view_char(r, 1));
	return r;
}

__private unsigned emit_deferr(emit_s* e, lipsAst_s* n){
	dbg_info("");
	switch( n->id ){
		default : die("emit: unknow rule %s as child of errdef", e->byc->name[n->id]);
		
		case LGRAM_word:{
			unsigned iderr = 0;
			lccfn_s* fn = lcc_get_function_byname(&e->lc, (const char*)n->tp, n->len);
			if( fn ){
				if( fn->iderr == 0 )  die("emit: syntax rule '%s' not have definition error ",  n->tp);
				iderr = fn->iderr;
			}
			else if( n->len != 7 || strncmp((const char*)n->tp, "_reset_", 7) ){
				die("emit: unable to define error linked on syntax rule '%s'",  n->tp);
			}
			return iderr;
		}
		
		case LGRAM_quoted:{
			long ret = lcc_error_add(&e->lc, (const char*)n->tp, n->len);
			if( ret < 0 ) lcc_err_die(&e->lc);
			return ret;
		}
	}
}

__private void emit_syntax_flags(emit_s* e, lipsAst_s* n, unsigned* store, unsigned* match, unsigned* err){
	dbg_info("");
	*store = 1;
	*match = 0;
	*err   = 0;
	if( !n ) return;
	mforeach(n->child, i){
		switch( n->child[i]->id ){
			default                  : die("emit: unknow rule %s as child of syntax_flags", e->byc->name[n->child[i]->id]);
			case LGRAM_syntax_unstore: *store = 0; break;
			case LGRAM_syntax_match  : *match = 1; break;
			case LGRAM_errdef        :
				iassert( m_header(n->child[i]->child)->len == 1 );
				*err = emit_deferr(e, n->child[i]->child[0]);
			break;
		}
	}
}

__private void emit_syntax_def(emit_s* e, lipsAst_s* n){
	unsigned store, match, len, err;
	const utf8_t* name = get_token_string_or_die(e, n, 0, LGRAM_word, &len);
	emit_syntax_flags(e, get_token(n, 1, LGRAM_syntax_flags), &store, &match, &err);
	if( !strncmp((const char*)name, "_start_", len) ){
		dbg_info("fn _start_");
		lcc_start(&e->lc, 0);
		e->isstart = 1;
	}
	else{
		lcc_fn_prolog(&e->lc, (const char*)name, len, store, err, NULL, 0);
		dbg_info("fn[%u] %.*s", e->lc.currentFN, len, name);
		e->lc.fn[e->lc.currentFN].userctx = (match << 1)|store;
	}
}

__private void emit_rx_class(emit_s* e, lipsAst_s* n){
	dbg_info("");
	lccrange_s r;
	lcc_range_ctor(&r);
	const unsigned count = m_header(n->child)->len;
	unsigned start = 0;
	unsigned rev = 0;
	if( get_token(n, start, LGRAM_rx_class_rev) ){
		++start;
		rev = 1;
	}
	for( unsigned i = start; i < count; ++i ){
		lipsAst_s* range = get_token_or_die(e, n, i, LGRAM_rx_range);
		unsigned re;
		switch( m_header(range->child)->len ){
			case 1:
				lcc_range_set(&r, 
					unescape_char(
						get_token_or_die(e, range, 0, LGRAM_rx_char)
					)
				);
			break;
			case 2:
				re = unescape_char(get_token_or_die(e, range, 1, LGRAM_rx_char));
				for( unsigned j = unescape_char(get_token_or_die(e, range, 0, LGRAM_rx_char)); j <= re; ++j )
					lcc_range_set(&r, j); 
			break;
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
	dbg_info("");
	unsigned unnamed = 0;
	unsigned start = 0;
	if( get_token(n, start, LGRAM_rx_unnamed) ){
		unnamed = 1;
		++start;
	}
	else{
		lcc_save(&e->lc, e->save++);
	}
	emit_rx_altern(e, get_token_or_die(e, n, start, LGRAM_rx_altern));
	if( !unnamed ) lcc_save(&e->lc, e->save++);
}

__private void emit_rx_primary(emit_s* e, lipsAst_s* n){
	dbg_info("");
	n = n->child[0];
	switch( n->id ){
		default              : die("emit: unknow rule %s as child of rx_primary", e->byc->name[n->id]);
		case LGRAM_rx_char   : lcc_char(&e->lc, unescape_char(n)); break;
		case LGRAM_rx_any    : lcc_range(&e->lc, 0); break;
		case LGRAM_rx_class  : emit_rx_class(e, n); break;
		case LGRAM_rx_group  : emit_rx_group(e, n); break;
	}
}

__private void emit_rx_repeat(emit_s* e, lipsAst_s* n){
	dbg_info("");
	quantifier_emit(e, n, emit_rx_primary, LGRAM_rx_primary);
}

__private void emit_rx_concat(emit_s* e, lipsAst_s* n){
	mforeach(n->child, i){
		dbg_info("%u", i);
		emit_rx_repeat(e, get_token_or_die(e, n, i, LGRAM_rx_repeat));
	}
}

__private void emit_rx_altern(emit_s* e, lipsAst_s* n){
	dbg_info("");
	choose_token_emit(e, n, emit_rx_concat, LGRAM_rx_concat);
}

__private void emit_regex(emit_s* e, lipsAst_s* n){
	dbg_info("");
	emit_rx_altern(e, get_token_or_die(e, n, 0, LGRAM_rx_altern));
}

__private void emit_syntax_altern(emit_s* e, lipsAst_s* n);
__private void emit_syntax_primary(emit_s* e, lipsAst_s* n){
	dbg_info("");
	n = n->child[0];
	switch( n->id ){
		case LGRAM_regex       : emit_regex(e, n); break;
		case LGRAM_errdef      : lcc_error(&e->lc, emit_deferr(e, n->child[0])); break;
		case LGRAM_syntax_group: emit_syntax_altern(e, get_token_or_die(e, n, 0, LGRAM_syntax_altern)); break;
		case LGRAM_word        : lcc_call(&e->lc, (const char*)n->tp, n->len); break;
		default: die("emit: unknow rule %s as child of syntax_primary", e->byc->name[n->id]);
	}
}

__private void emit_syntax_repeat(emit_s* e, lipsAst_s* n){
	dbg_info("");
	quantifier_emit(e, n, emit_syntax_primary, LGRAM_syntax_primary);
}

__private void emit_syntax_concat(emit_s* e, lipsAst_s* n){
	mforeach(n->child, i){
		dbg_info("%u", i);
		emit_syntax_repeat(e, get_token_or_die(e, n, i, LGRAM_syntax_repeat));
	}
}

__private void emit_syntax_altern(emit_s* e, lipsAst_s* n){
	dbg_info("");
	choose_token_emit(e, n, emit_syntax_concat, LGRAM_syntax_concat);
}

__private void emit_syntax(emit_s* e, lipsAst_s* n){
	dbg_info("");
	emit_syntax_def(e, get_token_or_die(e, n, 0, LGRAM_syntax_def));
	emit_syntax_altern(e, get_token_or_die(e, n, 1, LGRAM_syntax_altern));
	dbg_info("emit function[%u] epilog", e->lc.currentFN);
	if( !e->isstart ){
		unsigned flags = e->lc.fn[e->lc.currentFN].userctx;
	   	if( flags & 0x2 ) lcc_match(&e->lc, 1, 0);
		lcc_fn_epilog(&e->lc, flags & 1);
	}
	else{
		e->isstart = 0;
	}
}

__private void emit_lips(emit_s* e, lipsAst_s* n){
	mforeach( n->child, j ){
		switch( n->child[j]->id ){
			default                 : die("emit: unknow rule %s in as child of lips", e->byc->name[n->child[j]->id]);
			case LGRAM_syntax       : emit_syntax(e, n->child[j]); break;
		}
	}
}

__private void emit_grammar(emit_s* e, lipsAst_s* n){
	mforeach(n->child, i){
		dbg_warning("emit ast[%u]", i);
		iassert(n->child[i]->id == LGRAM_grammar);
		lipsAst_s* g = n->child[i];
		mforeach( g->child, j ){
			switch( g->child[j]->id ){
				case LGRAM_lips:
					emit_lips(e, g->child[j]);
				break;
				default: die("emit: unknow rule %s in as child of grammar", e->byc->name[g->child[j]->id]);
			}
		}
	}
}

uint16_t* lips_builtin_emitter(lipsByc_s* byc, lipsAst_s* ast){
	emit_s emit;
	emit.byc = byc;
	emit.ast = ast;
	emit.save = 2;
	emit.isstart = 0;
	lcc_ctor(&emit.lc);
	emit_grammar(&emit, emit.ast);
	uint16_t* ret = lcc_make(&emit.lc);
	dbg_info("bytecode size: %u", m_header(ret)->len);
	if( !ret ) lcc_err_die(&emit.lc);
	lcc_dtor(&emit.lc);
	return ret;
}


