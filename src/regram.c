#include <notstd/utf8.h>
#include <notstd/memory.h>
#include <mfmrecom.h>
#include <notstd/str.h>

/*
GRAMMAR: regex

opchar : '[|*+?(){}[]];
literal: '[^|*+?(){}[].];
escaped: '\[|*+?(){}[].\^$]';
char   : escaped
       | literal
;
number : [0-9]+;
begin  : '^';
end    : '$';
qtype  : [*+?]
lnum   : number;
rnum   : number;
qspec  : '{' lnum (',' rnum?)? '}';
quanti : qtype
       | qspec
;
chrange: char ('-' char)?;
chclass: '[' chrange+ ']';
group  : '(' altern ')'
primary: literal
       | group
       | chclass
       | escaped
       | '.'
;
repet  : primary ( quanti )?;
concat : repet+;
altern : concat ( '|' concat )*;
regex  : '/' begin? altern end?'/'
_start_: regex;

*/

__private unsigned fn_prolog(recom_s* rc, const char* name, unsigned store){
	INIT(rc);
	unsigned f = FN(name, 0);
	if( store ) NODE(f);
	return f;
}

__private void fn_epilog(recom_s* rc, unsigned stored){
	INIT(rc);
	if( stored ) PARENT();
	RET();
}

__private unsigned token_range(recom_s* rc, const char* accept, unsigned rev){
	INIT(rc);
	USERANGE();
	RRANGE();
	for( unsigned i = 0; accept[i]; ++i ) RRSET(accept[i]);
	if( rev ) RRREV();
	return RRADD();
}

__private void token_class(recom_s* rc, const char* name, const char* accept, unsigned rev, unsigned quantifier){
	INIT(rc);
	unsigned r = token_range(rc, accept, rev);
	fn_prolog(rc, name, 1);
	switch( quantifier ){
		case '0': RANGE(r); break;
		case '1':{ //+
			USELBL(1);
			LABEL(L[0]); RANGE(r);
			SPLIR(L[0]);
		}
		break;
	}
	fn_epilog(rc, 1);
}

__private void token_rename(recom_s* rc, const char* oldname, const char* newname){
	INIT(rc);
	fn_prolog(rc, newname, 1);
	CALL(oldname, 0);
	fn_epilog(rc, 1);
}

//opchar : '[|*+?(){}[]];
__private void def_opchar(recom_s* rc){
	token_class(rc, "opchar", "|*+?(){}[]", 0, 0);
}

//literal: '[^|*+?(){}[].];
__private void def_literal(recom_s* rc){
	token_class(rc, "literal", "|*+?(){}[]\0", 0, 0);
}

//escaped: '\[|*+?(){}[].\^$]';
__private void def_escaped(recom_s* rc){
	INIT(rc);
	unsigned r = token_range(rc, "|*+?(){}[].^$", 0);
	fn_prolog(rc, "escaped", 1);
	CHAR('\\');
	RANGE(r);
	fn_epilog(rc, 1);
}

//char   : escaped
//       | literal
//;
__private void def_char(recom_s* rc){
	INIT(rc);
	USELBL(2);
	fn_prolog(rc, "char", 1);
	SPLIT(L[0]);
	CALL("escaped", 0);
	JMP(L[1]);
	LABEL(L[0]); CALL("literal",0);
	LABEL(L[1]); fn_epilog(rc, 1);
}

//number : [0-9]+;
__private void def_number(recom_s* rc){
	token_class(rc, "number", "0123456789", 0, 1);
}

//begin  : '^';
__private void def_begin(recom_s* rc){
	INIT(rc);
	fn_prolog(rc, "begin", 1);
	CHAR('^');
	fn_epilog(rc, 1);
}

//end    : '$';
__private void def_end(recom_s* rc){
	INIT(rc);
	fn_prolog(rc, "end", 1);
	CHAR('$');
	fn_epilog(rc, 1);
}

//qtype  : [*+?]
__private void def_qtype(recom_s* rc){
	token_class(rc, "qtype", "*+?", 0, 0);
}

//lnum   : number;
__private void def_lnum(recom_s* rc){
	token_rename(rc, "number", "lnum");
}

//rnum   : number;
__private void def_rnum(recom_s* rc){
	token_rename(rc, "number", "rnum");
}

//qspec  : '{' lnum (',' rnum?)? '}';
__private void def_qspec(recom_s* rc){
	INIT(rc);
	USELBL(1);
	fn_prolog(rc, "qspec", 1);
	CHAR('{');
	CALL("lnum", 0);
	SPLIT(L[0]);
	CHAR(',');
	SPLIT(L[0]);
	CALL("rnum", 0);
	LABEL(L[0]); CHAR('}');
	fn_epilog(rc, 1);
}

//quanti : qtype
//       | qspec
//;
__private void def_quanti(recom_s* rc){
	INIT(rc);
	USELBL(2);
	fn_prolog(rc, "quanti", 1);
	SPLIT(L[0]);
	CALL("qtype", 0);
	JMP(L[1]);
	LABEL(L[0]); CALL("qspec", 0);
	LABEL(L[1]); fn_epilog(rc, 1);
}

//chrange: char ('-' char)?;
__private void def_chrange(recom_s* rc){
	INIT(rc);
	USELBL(1);
	fn_prolog(rc, "chrange", 1);
	CALL("char", 0);
	SPLIT(L[0]);
	CHAR('-');
	CALL("char", 0);
	LABEL(L[0]); fn_epilog(rc, 1);
}

//chclass: '[' chrange+ ']';
__private void def_chclass(recom_s* rc){
	INIT(rc);
	USELBL(1);
	fn_prolog(rc, "chclass", 1);
	CHAR('[');
	LABEL(L[0]); CALL("chrange", 0);
	SPLIR(L[0]);
	CHAR(']');
	fn_epilog(rc, 1);
}

//group  : '(' altern ')'
__private void def_group(recom_s* rc){
	INIT(rc);
	fn_prolog(rc, "group", 1);
	CHAR('(');
	CALL("altern", 0);
	CHAR(')');
	fn_epilog(rc, 1);
}

//primary: literal
//       | group
//       | chclass
//       | escaped
//       | '.'
//;
__private void def_primary(recom_s* rc){
	INIT(rc);
	USELBL(5);
	fn_prolog(rc, "primary", 1);
	SPLIT(L[1]);
	CALL("literal", 0);
	JMP(L[0]);
	LABEL(L[1]); SPLIT(L[2]);
	CALL("group", 0);
	JMP(L[0]);
	LABEL(L[2]); SPLIT(L[3]);
	CALL("chclass", 0);
	JMP(L[0]);
	LABEL(L[3]); SPLIT(L[4]);
	CALL("escaped", 0);
	JMP(L[0]);
	LABEL(L[4]); CHAR('.');
	LABEL(L[0]); fn_epilog(rc, 1);
}

//repet  : primary quanti?;
__private void def_repet(recom_s* rc){
	INIT(rc);
	USELBL(1);
	fn_prolog(rc, "repet", 1);
	CALL("primary", 0);
	SPLIT(L[0]);
	CALL("quanti", 0);
	LABEL(L[0]); fn_epilog(rc, 1);
}

//concat : repet+;
__private void def_concat(recom_s* rc){
	INIT(rc);
	USELBL(1);
	fn_prolog(rc, "concat", 1);
	LABEL(L[0]); CALL("repeat", 0);
	SPLIR(L[0]); fn_epilog(rc, 1);
}

//altern : concat ( '|' concat )*;
__private void def_altern(recom_s* rc){
	INIT(rc);
	USELBL(2);
	fn_prolog(rc, "altern", 1);
	LABEL(L[1]); CALL("concat", 0);
	SPLIT(L[0]);
	CHAR('|');
	CALL("concat", 0);
	JMP(L[1]);
	LABEL(L[0]); fn_epilog(rc, 1);
}

//regex  : '/' begin? altern end?'/'
__private void def_regex(recom_s* rc){
	INIT(rc);
	USELBL(2);
	fn_prolog(rc, "regex", 1);
	CHAR('/');
	SPLIT(L[0]);
	CALL("begin", 0);
	LABEL(L[0]); CALL("altern", 0);	
	SPLIT(L[1]);
	CALL("end", 0);
	LABEL(L[1]); CHAR('/');
	fn_epilog(rc, 1);
}

//_start_: regex;
uint16_t* regram_make(void){
	uint16_t* ret = NULL;
	CTOR();
	INIT(ROBJ());
	def_opchar(ROBJ());
	def_literal(ROBJ());
	def_escaped(ROBJ());
	def_char(ROBJ());
	def_number(ROBJ());
	def_begin(ROBJ());
	def_end(ROBJ());
	def_qtype(ROBJ());
	def_lnum(ROBJ());
	def_rnum(ROBJ());
	def_qspec(ROBJ());
	def_quanti(ROBJ());
	def_chrange(ROBJ());
	def_chclass(ROBJ());
	def_group(ROBJ());
	def_primary(ROBJ());
	def_repet(ROBJ());
	def_concat(ROBJ());
	def_altern(ROBJ());
	def_regex(ROBJ());
	START(0);
	ret = MAKE();
	DTOR();
	return ret;
}






















