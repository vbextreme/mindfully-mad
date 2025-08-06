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
	RRSTR("|*+?(){}[].^$\\/tn0", 0);
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
		CHOOSE_BEGIN(4);
			CALL("rx_char");
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

//syntax@['invalid syntax rule']                  : syntax_def skip syntax_altern rule_end;
__private void def_syntax(lcc_s* lc){
	INIT(lc);
	FN("syntax", 1, "invalid syntax rule"){
		CALL("syntax_def");
		CALL("skip");
		CALL("syntax_altern");
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
//		OR2(
			CALL("syntax");
//		,
//			CALL("semantic");
//		);
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
	def_lips(ROBJ());
	def_grammar_end(ROBJ());
	def_grammar(ROBJ());
	def_start(ROBJ());
	uint16_t* ret;
	MAKE(ret);
	DTOR();
	return ret;
}


















