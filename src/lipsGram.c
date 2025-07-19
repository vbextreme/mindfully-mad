#include <lips/ccompiler.h>
/* breakpoint list
 * 123 regex
 * 126 rule_group
 * 132 rule_primary
 *
*/
/* 14E -> 123 regex -> 7A primary
@error[1] 'unknown symbol at this state';

num       : /[0-9]+/;
lnum      : num;
rnum      : num;
qtype     : /[\*\+\?]/;
qspec     : /\{/ lnum (/,/ rnum?)? /\}/;
quantifier: qtype
          | qspec
          ;

rx_literal : /[^\|\*\+\?\(\)\{\}\[\]\.\/\\]/;
rx_escaped : /\\[\|\*\+\?\(\)\{\}\[\]\.\^\$\\0\;\/]/;
rx_char    : rx_escaped
           | rx_literal
           ;
rx_range   : rx_char ('-' rx_char)?;
rx_class   : '[' rx_range+ ']';

rx_begin : /\^/;
rx_end   : /\$/;
rx_any   : /./;

rx_unnamed: /\?:/;
rx_group  : /\(/ rx_unnamed? rx_altern /\)/;
rx_primary: rx_literal
          | rx_escaped
          | rx_group
          | rx_class
          | rx_any
          ;

rx_repeat : rx_primary quantifier?;
rx_concat : rx_repeat+;
rx_altern : rx_concat ( '|' rx_concat )*;
regex     : /\// rx_begin? rx_altern rx_end? /\//;

sep-      : /[ \t\n]+/;
skip-     : /[ \t\n]* /;
rule_end- : skip /\;/;
unstore   : /-/;
match     : /@/;
rulestart : /:/;
word      : /[a-zA-Z_][a-zA-Z_0-9]* /;
quoted    : /'(?:[^\\']|\\.)*'/;

builtin_error: /@ERROR\[/ num /\]/ sep quoted rule_end;

rule_flags  : unstore? match?;
rule_def    : skip word rule_flags skip rulestart;
rule_group  : /\(/ skip rule_altern skip /\)/;
rule_binerr : /$\[/ num /\]/;
rule_primary: regex
            | rule_group
            | rule_binerr
            | word
            ;
rule_repeat : skip rule_primary quantifier?;
rule_concat : rule_repeat+ rule_end;
rule_altern : rule_concat ( '|' rule_concat )*;
rule        : rule_def skip rule_altern;

lips: builtin_error
    | rule
    ;

grammar_end-@: /\0/;
grammar: regex
       | lips
       | grammar_end
       | $[1]
       ;

_start_: grammar+;

*/

__private void token_class(lcc_s* lc, const char* fnname, const char* accept, unsigned rev, unsigned store, unsigned mode){
	INIT(lc);
	USERANGE();
	RRSTR(accept, rev);
	unsigned ir = RRADD();
	FN(fnname, store){
		switch( mode ){
			case 0: RANGE(ir); break;
			case 1: ZMQ(RANGE(ir);); break;
			case 2: OMQ(RANGE(ir);); break;
		}
	}
}

__private void token_char(lcc_s* lc, const char* fnname, const char* accept, unsigned store){
	INIT(lc);
	FN(fnname, store){
		while( *accept ){
			CHAR(*accept++);
		}
	}
}

__private void token_replace(lcc_s* lc, const char* org, const char* newname){
	INIT(lc);
	FN(newname, 1){
		CALL(org);
	}
}

//num       : /[0-9]+/;
__private void def_num(lcc_s* lc){
	token_class(lc, "num", "0123456789", 0, 1, 2);
}

//lnum      : num;
__private void def_lnum(lcc_s* lc){
	token_replace(lc, "num", "lnum");
}

//lnum      : num;
__private void def_rnum(lcc_s* lc){
	token_replace(lc, "num", "rnum");
}

//qtype     : /[\*\+\?]/;
__private void def_qtype(lcc_s* lc){
	token_class(lc, "qtype", "*+?", 0, 1, 0);
}

//qspec     : /{/ lnum (/,/ rnum?)? /}/;
__private void def_qspec(lcc_s* lc){
	INIT(lc);
	FN("qspec", 1){
		CHAR('{');
		CALL("lnum");
		ZOQ(
			CHAR(',');
			ZOQ( CALL("rnum"); );
		);
		CHAR('}');
	}
}

//quantifier: qtype
//          | qspec
//          ;
__private void def_quantifier(lcc_s* lc){
	INIT(lc);
	FN("quantifier", 1){
		OR2(
			CALL("qtype");
		,
			CALL("qspec");
		);
	}
}

//rx_literal : /[^\|\*\+\?\(\)\{\}\[\]\.\/];
__private void def_rx_literal(lcc_s* lc){
	token_class(lc, "rx_literal", "|*+?(){}[]./\\", 1, 1, 0);
}

//rx_escaped : /\\[\|\*\+\?\(\)\{\}\[\]\.\^\$\\0\;\/]/;
__private void def_rx_escaped(lcc_s* lc){
	INIT(lc);
	USERANGE();
	RRSTR("|*+?(){}[].^$;\\0/", 0);
	unsigned ir = RRADD();
	FN("rx_escaped", 1){
		CHAR('\\');
		RANGE(ir);
	}
}

//rx_char    : rx_escaped
//           | rx_literal
//           ;
__private void def_rx_char(lcc_s* lc){
	INIT(lc);
	FN("rx_char", 1){
		OR2(
			CALL("rx_escaped");
		,
			CALL("rx_literal");
		);
	}
}

//rx_range   : rx_char ('-' rx_char)?;
__private void def_rx_range(lcc_s* lc){
	INIT(lc);
	FN("rx_range", 1){
		CALL("rx_char");
		ZOQ(
			CHAR('-');
			CALL("rx_char");
		);
	}
}

//rx_class   : '[' rx_range+ ']';
__private void def_rx_class(lcc_s* lc){
	INIT(lc);
	FN("rx_class", 1){
		CHAR('[');
		OMQ(
			CALL("rx_range");
		);
		CHAR(']');
	}
}

//rx_begin : /\^/;
__private void def_rx_begin(lcc_s* lc){
	token_char(lc, "rx_begin", "^", 1);
}

//rx_end   : /\$/;
__private void def_rx_end(lcc_s* lc){
	token_char(lc, "rx_end", "$", 1);
}

//rx_any   : /./;
__private void def_rx_any(lcc_s* lc){
	token_char(lc, "rx_any", ".", 1);
}

//rx_unnamed : /\(\?:/;
__private void def_rx_unnamed(lcc_s* lc){
	INIT(lc);
	FN("rx_unnamed", 1){
		CHAR('?');
		CHAR(':');
	}
}

//rx_group  : /\(/ rx_unnamed? rx_altern /\)/;
__private void def_rx_group(lcc_s* lc){
	INIT(lc);
	FN("rx_group", 1){
		CHAR('(');
		ZOQ(CALL("rx_unnamed"););
		CALL("rx_altern");
		CHAR(')');
	}
}

//rx_primary: rx_literal
//          | rx_escaped
//          | rx_group
//          | rx_class
//          | rx_any
//          ;
__private void def_rx_primary(lcc_s* lc){
	INIT(lc);
	FN("rx_primary", 1){
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

//rx_repeat : rx_primary quantifier?;
__private void def_rx_repeat(lcc_s* lc){
	INIT(lc);
	FN("rx_repeat", 1){
		CALL("rx_primary");
		ZOQ(CALL("quantifier"););
	}
}

//rx_concat : rx_repeat+;
__private void def_rx_concat(lcc_s* lc){
	INIT(lc);
	FN("rx_concat", 1){
		OMQ(CALL("rx_repeat"););
	}
}

//rx_altern : rx_concat ( '|' rx_concat )*;
__private void def_rx_altern(lcc_s* lc){
	INIT(lc);
	FN("rx_altern", 1){
		CALL("rx_concat");
		ZMQ(
			CHAR('|');
			CALL("rx_concat");
		);
	}
}

//regex  : /\// rx_begin? rx_altern rx_end? /\//;
__private void def_regex(lcc_s* lc){
	INIT(lc);
	FN("regex", 1){
		CHAR('/');
		ZOQ(CALL("rx_begin"););
		CALL("rx_altern");
		ZOQ(CALL("rx_end"););
		CHAR('/');
	}
}

//sep-      : /[ \t\n]+/;
__private void def_sep(lcc_s* lc){
	token_class(lc, "sep", " \t\n", 0, 0, 2);
}

//skip-     : /[ \t\n]* /;
__private void def_skip(lcc_s* lc){
	token_class(lc, "skip", " \t\n", 0, 0, 1);
}

//rule_end- : skip /\;/;
__private void def_rule_end(lcc_s* lc){
	INIT(lc);
	FN("rule_end", 0){
		CALL("skip");
		CHAR(';');
	}
}

//unstore   : /-/;
__private void def_unstore(lcc_s* lc){
	token_char(lc, "unstore", "-", 1);
}

//match     : /@/;
__private void def_match(lcc_s* lc){
	token_char(lc, "match", "@", 1);
}

//rulestart : /:/;
__private void def_rulestart(lcc_s* lc){
	token_char(lc, "rulestart", ":", 1);
}

//word      : /[a-zA-Z_][a-zA-Z_0-9]* /;
__private void def_word(lcc_s* lc){
	INIT(lc);
	USERANGE();
	RRSTR("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ_", 0);
	unsigned ra = RRADD();
	RRANGE();
	RRSTR("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ_0123456789", 0);
	unsigned rb = RRADD();
	FN("word", 1){
		RANGE(ra);
		ZMQ(RANGE(rb););
	}
}

//quoted    : /'(?:[^\\']|\\.)*'/;
__private void def_quoted(lcc_s* lc){
	INIT(lc);
	USERANGE();
	RRSET('\\');
	RRSET('\'');
	RRREV();
	unsigned nbl = RRADD();
	FN("quoted", 1){
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

//builtin_error: /@error\[/ num /\]/ sep quoted rule_end;
__private void def_builtin_error(lcc_s* lc){
	INIT(lc);
	FN("builtin_error", 1){
		CHAR('@');
		CHAR('e');
		CHAR('r');
		CHAR('r');
		CHAR('o');
		CHAR('r');
		CHAR('[');
		CALL("num");
		CHAR(']');
		CALL("sep");
		CALL("quoted");
		CALL("rule_end");
	}
}

//rule_flags  : unstore? match?;
__private void def_rule_flags(lcc_s* lc){
	INIT(lc);
	FN("rule_flags", 1){
		ZOQ(CALL("unstore"););
		ZOQ(CALL("match"););
	}
}

//rule_def    : skip word rule_flags skip rulestart;
__private void def_rule_def(lcc_s* lc){
	INIT(lc);
	FN("rule_def", 1){
		CALL("skip");
		CALL("word");
		CALL("rule_flags");
		CALL("skip");
		CALL("rulestart");
	}
}

//rule_group  : /\(/ skip rule_altern skip /\)/;
__private void def_rule_group(lcc_s* lc){
	INIT(lc);
	FN("rule_group", 1){
		CHAR('(');
		CALL("skip");
		CALL("rule_altern");
		CALL("skip");
		CHAR(')');
	}
}

//rule_binerr : /$\[/ num /\]/;
__private void def_rule_binerr(lcc_s* lc){
	INIT(lc);
	FN("rule_binerr", 1){
		CHAR('$');
		CHAR('[');
		CALL("num");
		CHAR(']');
	}
}

//rule_primary: regex
//            | rule_group
//            | rule_binerr
//            | word
//            ;
__private void def_rule_primary(lcc_s* lc){
	INIT(lc);
	FN("rule_primary", 1){
		CHOOSE_BEGIN(4);
		CALL("regex");
		CHOOSE();
		CALL("rule_group");
		CHOOSE();
		CALL("rule_binerr");
		CHOOSE();
		CALL("word");
		CHOOSE_END();
	}
}

//rule_repeat : skip rule_primary quantifier?;
__private void def_rule_repeat(lcc_s* lc){
	INIT(lc);
	FN("rule_repeat", 1){
		CALL("skip");
		CALL("rule_primary");
		ZOQ(CALL("quantifier"););
	}
}

//rule_concat : rule_repeat+ rule_end;
__private void def_rule_concat(lcc_s* lc){
	INIT(lc);
	FN("rule_concat", 1){
		OMQ(CALL("rule_repeat"););
		CALL("rule_end");
	}
}

//rule_altern : rule_concat ( '|' rule_concat )*;
__private void def_rule_altern(lcc_s* lc){
	INIT(lc);
	FN("rule_altern", 1){
		CALL("rule_concat");
		ZMQ(
			CHAR('|');
			CALL("rule_concat");
		);
	}
}

//rule        : rule_def skip rule_altern;
__private void def_rule(lcc_s* lc){
	INIT(lc);
	FN("rule", 1){
		CALL("rule_def");
		CALL("skip");
		CALL("rule_altern");
	}
}

//lips: builtin_error
//    | rule
//    ;
__private void def_lips(lcc_s* lc){
	INIT(lc);
	FN("lips", 1){
		OR2(
			CALL("builtin_error");
		,
			CALL("rule");
		);
	}
}

//grammar_end-@: /\0/;
__private void def_grammar_end(lcc_s* lc){
	INIT(lc);
	FN("grammar_end", 0){
		CHAR('\0');
		MATCH();
	}
}

//grammar: regex
//       | lips
//       | grammar_end
//       | $[1]
//       ;
__private void def_grammar(lcc_s* lc){
	INIT(lc);
	FN("grammar", 1){
		CHOOSE_BEGIN(4);
		CALL("regex");
		CHOOSE();
		CALL("lips");
		CHOOSE();
		CALL("grammar_end");
		CHOOSE();
		ERROR(1);
		CHOOSE_END();
	}
}

//@ERROR[1] 'unknown symbol at this state';
__private void dec_error(lcc_s* lc){
	INIT(lc);
	ERRADD("unknown symbol at this state");
}

//_start_: grammar+;
uint16_t* lips_builtin_grammar_make(void){
	CTOR();
	INIT(ROBJ());
	def_num(ROBJ());
	def_lnum(ROBJ());
	def_rnum(ROBJ());
	def_qtype(ROBJ());
	def_qspec(ROBJ());
	def_quantifier(ROBJ());
	//def_rx_operator(ROBJ());
	def_rx_literal(ROBJ());
	def_rx_escaped(ROBJ());
	def_rx_char(ROBJ());
	def_rx_range(ROBJ());
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
	def_sep(ROBJ());
	def_skip(ROBJ());
	def_rule_end(ROBJ());
	def_unstore(ROBJ());
	def_match(ROBJ());
	def_rulestart(ROBJ());
	def_word(ROBJ());
	def_quoted(ROBJ());
	def_builtin_error(ROBJ());
	def_rule_flags(ROBJ());
	def_rule_def(ROBJ());
	def_rule_group(ROBJ());
	def_rule_binerr(ROBJ());
	def_rule_primary(ROBJ());
	def_rule_repeat(ROBJ());
	def_rule_concat(ROBJ());
	def_rule_altern(ROBJ());
	def_rule(ROBJ());
	def_lips(ROBJ());
	def_grammar_end(ROBJ());
	def_grammar(ROBJ());
	dec_error(ROBJ());
	START(0);
	OMQ(CALL("grammar"););
	uint16_t* ret;
	MAKE(ret);
	DTOR();
	return ret;
}


















