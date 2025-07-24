#include <lips/ccompiler.h>
/*

@error[1]  'aspected char \':\', declare new rule';
@error[2]  'invalid number';
@error[3]  'required ; at end of rule';
@error[4]  'invalid quantifier'
@error[5]  'invalid char'
@error[6]  'invalid range'
@error[7]  'invalid group'
@error[8]  'invalid primary'
@error[9]  'invalid quoted string'
@error[10] 'invalid regex'
@error[11] 'invalid rule flags'
@error[12] 'invalid rule'
@error[13] 'invalid lips'


num@[2]   : /[0-9]+/;
lnum      : num;
rnum      : num;
qtype     : /[\*\+\?]/;
qspec     : /\{/ lnum (/,/ rnum?)? /\}/;
quantifier@[4]: qtype
              | qspec
              ;

rx_literal : /[^\|\*\+\?\(\)\{\}\[\]\.\/\\]/;
rx_escaped : /\\[\|\*\+\?\(\)\{\}\[\]\.\^\$\\0\;\/tn]/;
rx_char@[5]: rx_escaped
           | rx_literal
           ;
rx_range   : rx_char (/-/ rx_char)?;
rx_class@[6]: /\[/ rx_range+ /\]/;

rx_begin : /\^/;
rx_end   : /\$/;
rx_any   : /./;

rx_unnamed: /\?:/;
rx_group@[7]: /\(/ rx_unnamed? rx_altern /\)/;
rx_primary@[8]: rx_literal
              | rx_escaped
              | rx_group
              | rx_class
              | rx_any
              ;

rx_repeat : rx_primary quantifier?;
rx_concat : rx_repeat+;
rx_altern : rx_concat ( /\|/ rx_concat )*;
regex@[10]: /\// rx_begin? rx_altern rx_end? /\//;

sep-      : /[ \t\n]+/;
skip-     : /[ \t\n]* /;
rule_end-@[3]: skip /\;/;
unstore   : /-/;
match     : /!/;
rulestart@[1] : /:/;
word      : /[a-zA-Z_][a-zA-Z_0-9]* /;
quoted@[9]: /'(?:[^\\']|\\.)*'/;

builtin_error: /@ERROR\[/ num /\]/ sep quoted rule_end;

rule_binerr : /@\[/ num /\]/;
rule_flags@[11]: unstore? match? rule_binerr?;
rule_def       : skip word rule_flags skip rulestart;
rule_group@[7]: /\(/ skip rule_altern skip /\)/;
rule_primary@[8]: regex
                | rule_group
                | rule_binerr
                | word
                ;
rule_repeat : skip rule_primary quantifier?;
rule_concat : rule_repeat+;
rule_altern : rule_concat ( skip /|/ rule_concat )*;
rule@[12]   : rule_def skip rule_altern rule_end;

lips@[13]: builtin_error
         | rule
         ;

grammar_end-!: skip  /\0/;
grammar: regex
       | lips
       | grammar_end
       ;

_start_: (grammar @[0])+;







TODO
##comment
#comment

##error syntax
@error[1] 'aspected char \':\', assign rule';
@[1]>grammar>lips>rule>rule_def>word('k')
                               &rule_flags>altro>ciao<esempio>
;

##semantic
#+(name)  promote token to name and add to token list
#=(name)  annotated name to token
#&+(name) new scope name
#&>(name) new child scope name
#&<       new parent scope
#&-       end scope
#?(name)  if promoted name is in token list in current scope
#?(name<) if promoted name is in token list in current or any parent scope
#|(name)  elseif
#|        else

#create global scope
%[0] &+(global)

#promote rule word to ruleName and push to definition token
%[1] grammar>lips>rule>rule_def>word+(ruleName);

if primary rule is word and is promoted to ruleName annotaded here to call
#else raise error is not a function
@error[2] 'rule is not declared'
%[1] rule_primary>word?(ruleName)=(call)
                      |@[2]
;

%[2] &-

##emitter
: > print on file
: ! jitter
: * binary write

$[0]>rule_primary>~call: > call $word

*/
__private void token_class(lcc_s* lc, const char* fnname, const char* accept, unsigned rev, unsigned store, unsigned mode, unsigned err){
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

__private void token_char(lcc_s* lc, const char* fnname, const char* accept, unsigned store, unsigned err){
	INIT(lc);
	FN(fnname, store, err){
		while( *accept ){
			CHAR(*accept++);
		}
	}
}

__private void token_replace(lcc_s* lc, const char* org, const char* newname, unsigned err){
	INIT(lc);
	FN(newname, 1, err){
		CALL(org);
	}
}

//num@[2]: /[0-9]+/;
__private void def_num(lcc_s* lc){
	token_class(lc, "num", "0123456789", 0, 1, 2, 2);
}

//lnum      : num;
__private void def_lnum(lcc_s* lc){
	token_replace(lc, "num", "lnum", 0);
}

//lnum      : num;
__private void def_rnum(lcc_s* lc){
	token_replace(lc, "num", "rnum", 0);
}

//qtype     : /[\*\+\?]/;
__private void def_qtype(lcc_s* lc){
	token_class(lc, "qtype", "*+?", 0, 1, 0,  0);
}

//qspec     : /{/ lnum (/,/ rnum?)? /}/;
__private void def_qspec(lcc_s* lc){
	INIT(lc);
	FN("qspec", 1, 0){
		CHAR('{');
		CALL("lnum");
		ZOQ(
			CHAR(',');
			ZOQ( CALL("rnum"); );
		);
		CHAR('}');
	}
}

//quantifier@[4]: qtype
//          | qspec
//          ;
__private void def_quantifier(lcc_s* lc){
	INIT(lc);
	FN("quantifier", 1, 4){
		OR2(
			CALL("qtype");
		,
			CALL("qspec");
		);
	}
}

//rx_literal : /[^\|\*\+\?\(\)\{\}\[\]\.\/];
__private void def_rx_literal(lcc_s* lc){
	token_class(lc, "rx_literal", "|*+?(){}[]./\\", 1, 1, 0, 0);
}

//rx_escaped : /\\[\|\*\+\?\(\)\{\}\[\]\.\^\$\\0\;\/tn]/;
__private void def_rx_escaped(lcc_s* lc){
	INIT(lc);
	USERANGE();
	RRSTR("|*+?(){}[].^$;\\0/tn", 0);
	unsigned ir = RRADD();
	FN("rx_escaped", 1, 0){
		CHAR('\\');
		RANGE(ir);
	}
}

//rx_char@[5]: rx_escaped
//           | rx_literal
//           ;
__private void def_rx_char(lcc_s* lc){
	INIT(lc);
	FN("rx_char", 1, 5){
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
	FN("rx_range", 1, 0){
		CALL("rx_char");
		ZOQ(
			CHAR('-');
			CALL("rx_char");
		);
	}
}

//rx_class@[6]: '[' rx_range+ ']';
__private void def_rx_class(lcc_s* lc){
	INIT(lc);
	FN("rx_class", 1, 6){
		CHAR('[');
		OMQ(
			CALL("rx_range");
		);
		CHAR(']');
	}
}

//rx_begin : /\^/;
__private void def_rx_begin(lcc_s* lc){
	token_char(lc, "rx_begin", "^", 1, 0);
}

//rx_end   : /\$/;
__private void def_rx_end(lcc_s* lc){
	token_char(lc, "rx_end", "$", 1, 0);
}

//rx_any   : /./;
__private void def_rx_any(lcc_s* lc){
	token_char(lc, "rx_any", ".", 1, 0);
}

//rx_unnamed : /\(\?:/;
__private void def_rx_unnamed(lcc_s* lc){
	INIT(lc);
	FN("rx_unnamed", 1, 0){
		CHAR('?');
		CHAR(':');
	}
}

//rx_group@[7]: /\(/ rx_unnamed? rx_altern /\)/;
__private void def_rx_group(lcc_s* lc){
	INIT(lc);
	FN("rx_group", 1, 7){
		CHAR('(');
		ZOQ(CALL("rx_unnamed"););
		CALL("rx_altern");
		CHAR(')');
	}
}

//rx_primary@[8]: rx_literal
//              | rx_escaped
//              | rx_group
//              | rx_class
//              | rx_any
//              ;
__private void def_rx_primary(lcc_s* lc){
	INIT(lc);
	FN("rx_primary", 1, 8){
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
	FN("rx_repeat", 1, 0){
		CALL("rx_primary");
		ZOQ(CALL("quantifier"););
	}
}

//rx_concat : rx_repeat+;
__private void def_rx_concat(lcc_s* lc){
	INIT(lc);
	FN("rx_concat", 1, 0){
		OMQ(CALL("rx_repeat"););
	}
}

//rx_altern : rx_concat ( '|' rx_concat )*;
__private void def_rx_altern(lcc_s* lc){
	INIT(lc);
	FN("rx_altern", 1, 0){
		CALL("rx_concat");
		ZMQ(
			CHAR('|');
			CALL("rx_concat");
		);
	}
}

//regex@[10]: /\// rx_begin? rx_altern rx_end? /\//;
__private void def_regex(lcc_s* lc){
	INIT(lc);
	FN("regex", 1, 10){
		CHAR('/');
		ZOQ(CALL("rx_begin"););
		CALL("rx_altern");
		ZOQ(CALL("rx_end"););
		CHAR('/');
	}
}

//sep-      : /[ \t\n]+/;
__private void def_sep(lcc_s* lc){
	token_class(lc, "sep", " \t\n", 0, 0, 2, 0);
}

//skip-     : /[ \t\n]* /;
__private void def_skip(lcc_s* lc){
	token_class(lc, "skip", " \t\n", 0, 0, 1, 0);
}

//rule_end-@[3]: skip /\;/;
__private void def_rule_end(lcc_s* lc){
	INIT(lc);
	FN("rule_end", 0, 3){
		CALL("skip");
		CHAR(';');
	}
}

//unstore   : /-/;
__private void def_unstore(lcc_s* lc){
	token_char(lc, "unstore", "-", 1, 0);
}

//match     : /!/;
__private void def_match(lcc_s* lc){
	token_char(lc, "match", "!", 1, 0);
}

//rulestart@[1]: /:/;
__private void def_rulestart(lcc_s* lc){
	token_char(lc, "rulestart", ":", 1, 1);
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
	FN("word", 1, 0){
		RANGE(ra);
		ZMQ(RANGE(rb););
	}
}

//quoted@[9]: /'(?:[^\\']|\\.)*'/;
__private void def_quoted(lcc_s* lc){
	INIT(lc);
	USERANGE();
	RRSET('\\');
	RRSET('\'');
	RRREV();
	unsigned nbl = RRADD();
	FN("quoted", 1, 9){
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
	FN("builtin_error", 1, 0){
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

//rule_flags@[11]: unstore? match? rule_binerr?;
__private void def_rule_flags(lcc_s* lc){
	INIT(lc);
	FN("rule_flags", 1, 11){
		ZOQ(CALL("unstore"););
		ZOQ(CALL("match"););
		ZOQ(CALL("rule_binerr"););
	}
}

//rule_def    : skip word rule_flags skip rulestart;
__private void def_rule_def(lcc_s* lc){
	INIT(lc);
	FN("rule_def", 1, 0){
		CALL("skip");
		CALL("word");
		CALL("rule_flags");
		CALL("skip");
		CALL("rulestart");
	}
}

//rule_group@[7]: /\(/ skip rule_altern skip /\)/;
__private void def_rule_group(lcc_s* lc){
	INIT(lc);
	FN("rule_group", 1, 7){
		CHAR('(');
		CALL("skip");
		CALL("rule_altern");
		CALL("skip");
		CHAR(')');
	}
}

//rule_binerr : /@\[/ num /\]/;
__private void def_rule_binerr(lcc_s* lc){
	INIT(lc);
	FN("rule_binerr", 1, 0){
		CHAR('@');
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
	FN("rule_primary", 1, 0){
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
	FN("rule_repeat", 1, 0){
		CALL("skip");
		CALL("rule_primary");
		ZOQ(CALL("quantifier"););
	}
}

//rule_concat : rule_repeat+ rule_end;
__private void def_rule_concat(lcc_s* lc){
	INIT(lc);
	FN("rule_concat", 1, 0){
		OMQ(CALL("rule_repeat"););
	}
}

//rule_altern : rule_concat ( skip /|/ rule_concat )*;
__private void def_rule_altern(lcc_s* lc){
	INIT(lc);
	FN("rule_altern", 1, 0){
		CALL("rule_concat");
		ZMQ(
			CALL("skip");
			CHAR('|');
			CALL("rule_concat");
		);
	}
}

//rule@[12]: rule_def skip rule_altern;
__private void def_rule(lcc_s* lc){
	INIT(lc);
	FN("rule", 1, 12){
		CALL("rule_def");
		CALL("skip");
		CALL("rule_altern");
		CALL("rule_end");
	}
}

//lips@[13]: builtin_error
//    | rule
//    ;
__private void def_lips(lcc_s* lc){
	INIT(lc);
	FN("lips", 1, 13){
		OR2(
			CALL("builtin_error");
		,
			CALL("rule");
		);
	}
}

//grammar_end-@: skip /\0/;
__private void def_grammar_end(lcc_s* lc){
	INIT(lc);
	FN("grammar_end", 0, 0){
		CALL("skip");
		CHAR('\0');
		MATCH();
	}
}

//grammar: regex
//       | lips
//       | grammar_end
//       ;
__private void def_grammar(lcc_s* lc){
	INIT(lc);
	FN("grammar", 1, 0){
		CHOOSE_BEGIN(3);
		CALL("regex");
		CHOOSE();
		CALL("lips");
		CHOOSE();
		CALL("grammar_end");
		CHOOSE_END();
	}
}

//@error[1] 'aspected char \':\', declare new rule';
//@error[2] 'invalid number';
//@error[3] 'required ; at end of rule';
__private void dec_error(lcc_s* lc){
	INIT(lc);
	ERRADD("aspected \':\', declare new rule");
	ERRADD("invalid number");
	ERRADD("required ; at end of rule");
}

//_start_: (grammar $reset)+;
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
	OMQ(
		CALL("grammar");
		ERROR(0);
	);
	uint16_t* ret;
	MAKE(ret);
	DTOR();
	return ret;
}


















