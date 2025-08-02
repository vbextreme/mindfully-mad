#include <lips/ccompiler.h>

__private void token_class(lcc_s* lc, const char* fnname, const char* accept, unsigned rev, unsigned store, unsigned mode, const char* err){
	INIT(lc);
	USERANGE();
	RRSTR(accept, rev);
	unsigned ir = RRADD();
	FN(fnname, store, err){
		switch( mode ){
			case 0: RANGE(ir); break;
			case 1: ZMQ(RANGE(ir);); break;
			case 2: OMQ(RANGE(ir);); break;
		}
	}
}

__private void token_char(lcc_s* lc, const char* fnname, const char* accept, unsigned store, const char* err){
	INIT(lc);
	FN(fnname, store, err){
		while( *accept ){
			CHAR(*accept++);
		}
	}
}

__private void token_replace(lcc_s* lc, const char* org, const char* newname, const char* err){
	INIT(lc);
	FN(newname, 1, err){
		CALL(org);
	}
}

/***********************/
/*** section.generic ***/
/***********************/

//num@['invalid number']                 : /[0-9]+/;
__private void def_num(lcc_s* lc){
	token_class(lc, "num", "0123456789", 0, 1, 2, "invalid number");
}

//comment-                               : skip /#[^\n]*\n/;
__private void def_comment(lcc_s* lc){
	INIT(lc);
	USERANGE();
	RRSET('\n');
	RRREV();
	unsigned r = RRADD();
	FN("comment", 0, NULL){
		CALL("skip");
		CHAR('#');
		ZMQ(RANGE(r););
		CHAR('\n');
	}
}

//sep-                                   : /[ \t\n]+/;
__private void def_sep(lcc_s* lc){
	token_class(lc, "sep", " \t\n", 0, 0, 2, NULL);
}

//skip-                                  : /[ \t\n]* /;
__private void def_skip(lcc_s* lc){
	token_class(lc, "skip", " \t\n", 0, 0, 1, NULL);
}


//rule_end-@['required ; at end of rule']: skip /;/;
__private void def_rule_end(lcc_s* lc){
	INIT(lc);
	FN("rule_end", 0, "required ; at end of rule"){
		CALL("skip");
		CHAR(';');
	}
}

//quoted@['invalid quoted string']       : /'(?:[^\\']|\\.)*'/;
__private void def_quoted(lcc_s* lc){
	INIT(lc);
	USERANGE();
	RRSET('\\');
	RRSET('\'');
	RRREV();
	unsigned nbl = RRADD();
	FN("quoted", 1, "invalid quoted string"){
		CHAR('\'');
		ZMQ(
			OR2(
				RANGE(nbl);
			,
				CHAR('\\');
				ANY();
			);
		);
		CHAR('\'');
	}
}

//word                                   : /[a-zA-Z_][a-zA-Z_0-9]* /;
__private void def_word(lcc_s* lc){
	INIT(lc);
	USERANGE();
	RRSTR("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ_", 0);
	unsigned ra = RRADD();
	RRANGE();
	RRSTR("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ_0123456789", 0);
	unsigned rb = RRADD();
	FN("word", 1, NULL){
		RANGE(ra);
		ZMQ(RANGE(rb););
	}
}

/*********************/
/*** section error ***/
/*********************/

//errdef@['invalid error definition']: /@\[/ (word|quoted) /\]/;
__private void def_errdef(lcc_s* lc){
	INIT(lc);
	FN("errdef", 1, "invalid error definition"){
		CHAR('@');
		CHAR('[');
		OR2(
			CALL("word");
		,
			CALL("quoted");
		);
		CHAR(']');
	}
}

/**************************/
/*** section.quantifier ***/
/**************************/

//lnum                             : num;
__private void def_lnum(lcc_s* lc){
	token_replace(lc, "num", "lnum", NULL);
}

//rnum                             : num;
__private void def_rnum(lcc_s* lc){
	token_replace(lc, "num", "rnum", NULL);
}

//lazy                             : /\?/
__private void def_lazy(lcc_s* lc){
	token_char(lc, "lazy", "?", 1, NULL);
}

//qtype                            : /[\*\+\?]/;
__private void def_qtype(lcc_s* lc){
	token_class(lc, "qtype", "*+?", 0, 1, 0,  NULL);
}

//qspec                            : /\{/ lnum (/,/ rnum?)? /\}/;
__private void def_qspec(lcc_s* lc){
	INIT(lc);
	FN("qspec", 1, NULL){
		CHAR('{');
		CALL("lnum");
		ZOQ(
			CHAR(',');
			ZOQ( CALL("rnum"); );
		);
		CHAR('}');
	}
}

//quantifier@['invalid quantifier']: qtype lazy?
//                                 | qspec
//                                 ;
__private void def_quantifier(lcc_s* lc){
	INIT(lc);
	FN("quantifier", 1, "invalid quantifier"){
		MARK();
		OR2(
			CALL("qtype");
			ZOQ(CALL("lazy"););
		,
			CALL("qspec");
		);
	}
}

/*********************/
/*** section.regex ***/
/*********************/

//rx_literal                    : /[^\|\*\+\?\(\)\{\}\[\]\.\/\\]/;
__private void def_rx_literal(lcc_s* lc){
	token_class(lc, "rx_literal", "|*+?(){}[]./\\", 1, 1, 0, NULL);
}

//rx_escaped                    : /\\[\|\*\+\?\(\)\{\}\[\]\.\^\$\\0\/tn]/;
__private void def_rx_escaped(lcc_s* lc){
	INIT(lc);
	USERANGE();
	RRSTR("|*+?(){}[].^$\\0/tn", 0);
	unsigned ir = RRADD();
	FN("rx_escaped", 1, NULL){
		CHAR('\\');
		RANGE(ir);
	}
}

//rx_char@['invalid char']      : rx_escaped
//                              | rx_literal
//                              ;
__private void def_rx_char(lcc_s* lc){
	INIT(lc);
	FN("rx_char", 1, "invalid char"){
		OR2(
			CALL("rx_escaped");
		,
			CALL("rx_literal");
		);
	}
}

//rx_range                      : rx_char (/-/ rx_char)?;
__private void def_rx_range(lcc_s* lc){
	INIT(lc);
	FN("rx_range", 1, NULL){
		CALL("rx_char");
		ZOQ(
			CHAR('-');
			CALL("rx_char");
		);
	}
}

//rx_class_rev                  : /\^/
__private void def_rx_class_rev(lcc_s* lc){
	token_char(lc, "rx_class_rev", "^", 1, NULL);
}

//rx_class@['invalid range']    : /\[/ rx_class_rev? rx_range+ /\]/;
__private void def_rx_class(lcc_s* lc){
	INIT(lc);
	FN("rx_class", 1, "invalid range"){
		CHAR('[');
		ZOQ(CALL("rx_class_rev"););
		OMQ(
			CALL("rx_range");
		);
		CHAR(']');
	}
}

//rx_begin                      : /\^/;
__private void def_rx_begin(lcc_s* lc){
	token_char(lc, "rx_begin", "^", 1, NULL);
}

//rx_end                        : /\$/;
__private void def_rx_end(lcc_s* lc){
	token_char(lc, "rx_end", "$", 1, NULL);
}

//rx_any                        : /./;
__private void def_rx_any(lcc_s* lc){
	token_char(lc, "rx_any", ".", 1, NULL);
}

//rx_unnamed                    : /\?:/;
__private void def_rx_unnamed(lcc_s* lc){
	INIT(lc);
	FN("rx_unnamed", 1, NULL){
		CHAR('?');
		CHAR(':');
	}
}

//rx_group@['invalid group']    : /\(/ rx_unnamed? rx_altern /\)/;
__private void def_rx_group(lcc_s* lc){
	INIT(lc);
	FN("rx_group", 1, "invalid group"){
		CHAR('(');
		ZOQ(CALL("rx_unnamed"););
		CALL("rx_altern");
		CHAR(')');
	}
}

//rx_primary@['invalid primary']: rx_literal
//                              | rx_escaped
//                              | rx_group
//                              | rx_class
//                              | rx_any
//                              ;
__private void def_rx_primary(lcc_s* lc){
	INIT(lc);
	FN("rx_primary", 1, "invalid primary"){
		CHOOSE_BEGIN(5);
			CALL("rx_literal");
		CHOOSE();
			CALL("rx_escaped");
		CHOOSE();
			CALL("rx_group");
		CHOOSE();
			CALL("rx_class");
		CHOOSE();
			CALL("rx_any");
		CHOOSE_END();
	}
}

//rx_repeat                     : rx_primary quantifier?;
__private void def_rx_repeat(lcc_s* lc){
	INIT(lc);
	FN("rx_repeat", 1, NULL){
		CALL("rx_primary");
		ZOQ(CALL("quantifier"););
	}
}

//rx_concat                     : rx_repeat+;
__private void def_rx_concat(lcc_s* lc){
	INIT(lc);
	FN("rx_concat", 1, NULL){
		OMQ(CALL("rx_repeat"););
	}
}

//rx_altern                     : rx_concat ( /\|/ rx_concat )*;
__private void def_rx_altern(lcc_s* lc){
	INIT(lc);
	FN("rx_altern", 1, NULL){
		CALL("rx_concat");
		ZMQ(
			CHAR('|');
			CALL("rx_concat");
		);
	}
}

//regex@['invalid regex']       : /\// rx_begin? rx_altern rx_end? /\//;
__private void def_regex(lcc_s* lc){
	INIT(lc);
	FN("regex", 1, "invalid regex"){
		CHAR('/');
		ZOQ(CALL("rx_begin"););
		CALL("rx_altern");
		ZOQ(CALL("rx_end"););
		CHAR('/');
	}
}

/**********************/
/*** section.syntax ***/
/**********************/

//syntax_unstore                                  : /-/;
__private void def_syntax_unstore(lcc_s* lc){
	token_char(lc, "syntax_unstore", "-", 1, NULL);
}

//syntax_match                                    : /!/;
__private void def_syntax_match(lcc_s* lc){
	token_char(lc, "syntax_match", "!", 1, NULL);
}

//syntax_start@['aspected syntax rule definition']: /:/;
__private void def_syntax_start(lcc_s* lc){
	token_char(lc, "syntax_start", ":", 1, "aspected syntax rule definition");
}

//syntax_flags@['invalid syntax flag']            : unstore? match? errdef?;
__private void def_syntax_flags(lcc_s* lc){
	INIT(lc);
	FN("syntax_flags", 1, "invalid syntax flags"){
		ZOQ(CALL("syntax_unstore"););
		ZOQ(CALL("syntax_match"););
		ZOQ(CALL("errdef"););
	}
}

//syntax_def                                      : skip word syntax_flags skip syntax_start;
__private void def_syntax_def(lcc_s* lc){
	INIT(lc);
	FN("syntax_def", 1, NULL){
		MARK();
		CALL("skip");
		CALL("word");
		CALL("syntax_flags");
		CALL("skip");
		CALL("syntax_start");
	}
}

//syntax_group@['invalid group']                  : /\(/ skip syntax_altern skip /\)/;
__private void def_syntax_group(lcc_s* lc){
	INIT(lc);
	FN("syntax_group", 1, "invalid group"){
		CHAR('(');
		CALL("skip");
		CALL("syntax_altern");
		CALL("skip");
		CHAR(')');
	}
}

//syntax_primary                                  : regex
//                                                | syntax_group
//                                                | errdef
//                                                | word
//                                                ;
__private void def_syntax_primary(lcc_s* lc){
	INIT(lc);
	FN("syntax_primary", 1, NULL){
		MARK();
		CHOOSE_BEGIN(4);
		CALL("regex");
		CHOOSE();
		CALL("syntax_group");
		CHOOSE();
		CALL("errdef");
		CHOOSE();
		CALL("word");
		CHOOSE_END();
	}
}

//syntax_repeat                                   : skip syntax_primary quantifier?;
__private void def_syntax_repeat(lcc_s* lc){
	INIT(lc);
	FN("syntax_repeat", 1, NULL){
		CALL("skip");
		CALL("syntax_primary");
		ZOQ(CALL("quantifier"););
	}
}

//syntax_concat                                   : syntax_repeat+;
__private void def_syntax_concat(lcc_s* lc){
	INIT(lc);
	FN("syntax_concat", 1, NULL){
		OMQ(CALL("syntax_repeat"););
	}
}

//syntax_altern                                   : syntax_concat ( skip /\|/ syntax_concat )*;
__private void def_syntax_altern(lcc_s* lc){
	INIT(lc);
	FN("syntax_altern", 1, NULL){
		CALL("syntax_concat");
		ZMQ(
			CALL("skip");
			CHAR('|');
			CALL("syntax_concat");
		);
	}
}

//syntax@['invalid syntax rule']                  : rule_def skip rule_altern rule_end;
__private void def_syntax(lcc_s* lc){
	INIT(lc);
	FN("syntax", 1, "invalid syntax rule"){
		CALL("rule_def");
		CALL("skip");
		CALL("rule_altern");
		CALL("rule_end");
	}
}

/************************/
/*** section.semantic ***/
/************************/

//semantic_value@['aspected quoted value']                                       : /=/ skip quoted;
__private void def_semantic_value(lcc_s* lc){
	INIT(lc);
	FN("semantic_value", 1, "aspected quoted value"){
		CHAR('=');
		CALL("skip");
		CALL("quoted");
	}
}

//semantic_promote@['aspected valid node']                                       : /\^/ skip word;
__private void def_semantic_promote(lcc_s* lc){
	INIT(lc);
	FN("semantic_promote", 1, "aspected valid node"){
		CHAR('^');
		CALL("skip");
		CALL("word");
	}
}

//semantic_symbol@[semantic_promote]                                             : /\+/ skip word;
__private void def_semantic_symbol(lcc_s* lc){
	INIT(lc);
	FN("semantic_symbol", 1, "aspected valid node"){
		CHAR('+');
		CALL("skip");
		CALL("word");
	}
}

//semantic_senter                                                                : /\+/;
__private void def_semantic_senter(lcc_s* lc){
	token_char(lc, "semantic_senter", "+", 1, NULL);
}

//semantic_sleave                                                                : /\-/;
__private void def_semantic_sleave(lcc_s* lc){
	token_char(lc, "semantic_sleave", "+", 1, NULL);
}

//semantic_scope@['invalid scope']                                               : /$/ (semantic_senter|semantic_sleave);
__private void def_semantic_scope(lcc_s* lc){
	INIT(lc);
	FN("semantic_scope", 1, "invalid scope"){
		CHAR('$');
		OR2(
			CALL("semantic_senter");
		,
			CALL("semantic_sleave");
		);
	}
}

//semantic_change@['invalid operation, aspected promotion, symbol or set value'] : semantic_promote
//                                                                               | semantic_symbol
//                                                                               | semantic_value
//                                                                               ;
__private void def_semmantic_change(lcc_s* lc){
	INIT(lc);
	FN("semantic_change", 1, "invalid operation, aspected promotion, symbol or set value"){
		CHOOSE_BEGIN(3);
		CALL("semantic_promote");
		CHOOSE();
		CALL("semantic_symbol");
		CHOOSE();
		CALL("semantic_value");
		CHOOSE_END();
	}
}

//semantic_query@['invalid query']                                               : /\?/ skip (word | quoted);
__private void def_semantic_query(lcc_s* lc){
	INIT(lc);
	FN("semantic_query", 1, "invalid query"){
		CHAR('?');
		CALL("skip");
		OR2(
			CALL("word");
		,
			CALL("quoted");
		);
	}
}

//semantic_primary                                                               : semantic_change
//                                                                               | semantic_query
//                                                                               | semantic_altern
//                                                                               ;
__private void def_semantic_primary(lcc_s* lc){
	INIT(lc);
	FN("semantic_primary", 1, NULL){
		OR3(
			CALL("semantic_change");
		,
			CALL("semantic_query");
		,
			CALL("semantic_altern");
		);
	}
}

//semantic_enter                                                                 : skip /\(/ skip semantic_primary skip /\)/ skip;
__private void def_semantic_enter(lcc_s* lc){
	INIT(lc);
	FN("semantic_enter", 1, NULL){
		CALL("skip");
		CHAR('(');
		CALL("skip");
		CALL("semantic_primary");
		CALL("skip");
		CHAR(')');
		CALL("skip");
	}
}

//semantic_child@['invalid child']                                               : skip word semantic_enter?;
__private void def_semantic_child(lcc_s* lc){
	INIT(lc);
	FN("semantic_child", 1, "invalid child"){
		CALL("skip");
		CALL("word");
		ZOQ(CALL("semantic_enter"););
	}
}

//semantic_repeat                                                                : semantic_child
__private void def_semantic_repeat(lcc_s* lc){
	token_replace(lc, "semantic_child", "semantic_repeat", NULL);
}

//semantic_concat                                                                : semantic_repeat+;
__private void def_semantic_concat(lcc_s* lc){
	INIT(lc);
	FN("semantic_concat", 1, NULL){
		OMQ(CALL("semantic_repeat"););
	}
}

//semantic_altern                                                                : semantic_concat ( skip /\|/ semantic_concat )*;
__private void def_semantic_altern(lcc_s* lc){
	INIT(lc);
	FN("semantic_altern", 1, NULL){
		CALL("semantic_concat");
		ZOQ(
			CALL("skip");
			CHAR('|');
			CALL("semantic_concat");
		);
	}
}

//semantic_rule                                                                  : skip word semantic_enter;
__private void def_semantic_rule(lcc_s* lc){
	INIT(lc);
	FN("semantic_rule", 1, NULL){
		CALL("skip");
		CALL("word");
		CALL("semantic_enter");
	}
}

//semantic_stage                                                                 : /\[/ num /\]/;
__private void def_semantic_stage(lcc_s* lc){
	INIT(lc);
	FN("semantic_stage", 1, NULL){
		CHAR('[');
		CALL("num");
		CHAR(']');
	}
}

//semantic@['invalid semantic rule']                                             : /%/ ( semantic_stage | semantic_rule ) rule_end;
__private void def_semantic(lcc_s* lc){
	INIT(lc);
	FN("semantic", 1, "invalid semantic rule"){
		CHAR('%');
		OR2( CALL("semantic_stage");, CALL("semantic_rule"); );
		CALL("rule_end");
	}
}

/********************/
/*** section.lips ***/
/********************/

//lips@['invalid lips rule']: syntax
//                          | semantic
//                          ;
__private void def_lips(lcc_s* lc){
	INIT(lc);
	FN("lips", 1, "invalid lips rule"){
		OR2(
			CALL("syntax");
		,
			CALL("semantic");
		);
	}
}

//grammar_end-!             : skip  /\0/;
__private void def_grammar_end(lcc_s* lc){
	INIT(lc);
	FN("grammar_end", 0, NULL){
		CALL("skip");
		CHAR('\0');
		MATCH();
	}
}

//grammar                   : regex
//                          | lips
//                          | comment
//                          | grammar_end
//                          ;
__private void def_grammar(lcc_s* lc){
	INIT(lc);
	FN("grammar", 1, NULL){
		MARK();
		CHOOSE_BEGIN(4);
		CALL("regex");
		CHOOSE();
		CALL("lips");
		CHOOSE();
		CALL("comment");
		CHOOSE();
		CALL("grammar_end");
		CHOOSE_END();
	}
}

//_start_                   : (skip grammar @[0])+;
__private void def_start(lcc_s* lc){
	INIT(lc);
	START(0);
	OMQ(
		CALL("skip");
		CALL("grammar");
		ERROR(0);
	);
}

/**************************/
/*** semantic.anal yzer ***/
/**************************/


//%syntax_def(
//	word(+ruleName)
//);
__private void def_syntax_def_promote_word_rulename(lcc_s* lc){
	INIT(lc);
	SEMRULE();
	NAME("ruleName");
	ENTER("syntax_def");
	ENTER("word");
	TYPE("ruleName");
	SCOPESYMBOL();
	SMATCH();
}

//%quantifier(
//	qspec(
//		lnum(?'0')
//		rnum(?'1')
//		^qtype
//		='?'
//    )
//);
__private void def_change_special_quantifier(lcc_s* lc){
	INIT(lc);
	SEMRULE();
	ENTER("quantifier");
	ENTER("qspec");
	ENTER("lnum");
	VALUET("0");
	LEAVE();
	ENTER("rnum");
	VALUET("1");
	LEAVE();
	TYPE("qtype");
	VALUES("?");
	SMATCH();
}

//%grammar(
//	lips
//	|
//	@['grammar accepted only lips rule']
//);
__private void def_grammar_accpet_only_lips(lcc_s* lc){
	INIT(lc);
	SEMRULE();
	ENTER("grammar");
	OR2(
		ENTER("lips");
		SMATCH();
	,
		ERROR(ERRADD("grammar accepted only lips rule"));
		EMATCH();
	);
}

//%[0];
__private void def_semantic_stage0(lcc_s* lc){
	INIT(lc);
	SEMFASE();
	def_syntax_def_promote_word_rulename(lc);
	def_change_special_quantifier(lc);
	def_grammar_accpet_only_lips(lc);
}

//%syntax_primary(
//	word(
//		?ruleName
//		^call
//	)
//	|
//	@['unable call rule, rule is not declared']
//);
__private void def_promote_primary_word_as_call(lcc_s* lc){
	INIT(lc);
	SEMRULE();
	NAME("call");
	ENTER("syntax_primary");
	OR2(
		ENTER("word");
		SYMBOL("ruleName");
		TYPE("call");
		SMATCH();
	,
		ERROR(ERRADD("rule is not declared"));
		EMATCH();
	);
}

//%[1];
__private void def_semantic_stage1(lcc_s* lc){
	INIT(lc);
	SEMFASE();
	def_promote_primary_word_as_call(lc);
}

uint16_t* lips_builtin_grammar_make(void){
	CTOR();
	INIT(ROBJ());
	def_num(ROBJ());
	def_comment(ROBJ());
	def_sep(ROBJ());
	def_skip(ROBJ());
	def_rule_end(ROBJ());
	def_quoted(ROBJ());
	def_word(ROBJ());
	def_errdef(ROBJ());
	def_lnum(ROBJ());
	def_rnum(ROBJ());
	def_lazy(ROBJ());
	def_qtype(ROBJ());
	def_qspec(ROBJ());
	def_quantifier(ROBJ());
	def_rx_literal(ROBJ());
	def_rx_escaped(ROBJ());
	def_rx_char(ROBJ());
	def_rx_range(ROBJ());
	def_rx_class_rev(ROBJ());
	def_rx_class(ROBJ());
	def_rx_begin(ROBJ());
	def_rx_end(ROBJ());
	def_rx_any(ROBJ());
	def_rx_unnamed(ROBJ());
	def_rx_group(ROBJ());
	def_rx_primary(ROBJ());
	def_rx_repeat(ROBJ());
	def_rx_concat(ROBJ());
	def_rx_altern(ROBJ());
	def_regex(ROBJ());
	def_syntax_unstore(ROBJ());
	def_syntax_match(ROBJ());
	def_syntax_start(ROBJ());
	def_syntax_flags(ROBJ());
	def_syntax_def(ROBJ());
	def_syntax_group(ROBJ());
	def_syntax_primary(ROBJ());
	def_syntax_repeat(ROBJ());
	def_syntax_concat(ROBJ());
	def_syntax_altern(ROBJ());
	def_syntax(ROBJ());
	def_semantic_value(ROBJ());
	def_semantic_promote(ROBJ());
	def_semantic_symbol(ROBJ());
	def_semantic_senter(ROBJ());
	def_semantic_sleave(ROBJ());
	def_semantic_scope(ROBJ());
	def_semmantic_change(ROBJ());
	def_semantic_query(ROBJ());
	def_semantic_primary(ROBJ());
	def_semantic_enter(ROBJ());
	def_semantic_child(ROBJ());
	def_semantic_repeat(ROBJ());
	def_semantic_concat(ROBJ());
	def_semantic_altern(ROBJ());
	def_semantic_rule(ROBJ());
	def_semantic_stage(ROBJ());
	def_semantic(ROBJ());
	def_lips(ROBJ());
	def_grammar_end(ROBJ());
	def_grammar(ROBJ());
	def_semantic_stage0(ROBJ());
	def_semantic_stage1(ROBJ());
	def_start(ROBJ());
	uint16_t* ret;
	MAKE(ret);
	DTOR();
	return ret;
}


















