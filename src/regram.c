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
       | escaped_char
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

//chclass: '[' chrange+ ']';

//group  : '(' altern ')'

//primary: literal
//       | group
//       | chclass
//       | escaped_char
//       | '.'
//;

//repet  : primary ( quanti )?;

//concat : repet+;

//altern : concat ( '|' concat )*;

//regex  : '/' begin? altern end?'/'

//_start_: regex;





















