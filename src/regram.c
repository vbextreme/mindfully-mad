#include <notstd/utf8.h>
#include <notstd/memory.h>
#include <mfmrecom.h>
#include <notstd/str.h>

/*
GRAMMAR: reasm;

num   : '[0-9]+';
word  : '[a-zA-Z_]+[a-zA-Z0-9]*';
endl- : '\n';
sep-  : '[ \t]+';
comma-: ';';
colon-: ':';
dot-  : '.';
any-  : '[^\n]*';
fn    : 'fn';
dec   : 'dec';
range : 'range';
escape: '\\[nt]';
nscape: '[^\n\t];
ch    : escape
      | num
      | nescape
      ;
char  : '\'' escape '\''
      | '\'' nescape '\''
      ;

emptyline-: sep? endl;
comment-  : comma any;
label     : word sep? colon;
decfn     : fn dot word sep? colon;
endcmd    : sep? comment
          | sep? endl
          ;
chrange   : ch ('-' ch)?;
chclass   : '\'' chrange+ '\'';
decrange  : dec dot range '[' num ']' sep? colon sep? chclass endcmd
arg   : char
      | num
      | word
      ;
cmdsym: word endcmd;
cmdarg: word sep arg endcmd;
cmd   : sep? cmdsym
      | sep? cmdarg
      ;
cmdlbl: label cmd;
cmdfn : decfn cmd;

command: sep? cmdfn
       | sep? cmdlbl
       | cmd
       | sep? decrange
       ;

progline-: emptyline
         | sep? comment
         | command
         ;

program: progline+;
_start_: program;
*/

__private void token_class(recom_s* rc, const char* fnname, const char* accept, unsigned rev, unsigned store){
	INIT(rc);
	FN(fnname, store);
	USERANGE();
	USELBL(1);
	RRANGE();
	RRSTR(accept, rev);
	unsigned ir = RRADD();
	LABEL(L[0]); RANGE(ir);
	SPLIR(L[0]);
	RET(store);
}

__private void token_char(recom_s* rc, const char* fnname, const char* accept, unsigned store){
	INIT(rc);
	FN(fnname, store);
	while( *accept ){
		CHAR(*accept++);
	}
	RET(store);
}

//num   : '[0-9]+';
__private void def_num(recom_s* rc){
	token_class(rc, "num", "0123456789", 0, 1);
}

//word  : '[a-zA-Z_]+[a-zA-Z0-9]*';
__private void def_word(recom_s* rc){
	INIT(rc);
	FN("word", 1);
	USERANGE();
	USELBL(3);
	RRANGE();
	RRSTR("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ_", 0);
	unsigned ist = RRADD();
	RRANGE();
	RRSTR("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ_0123456789", 0);
	unsigned icn = RRADD();
	LABEL(L[0]); RANGE(ist);
	SPLIR(L[0]);
	LABEL(L[1]); SPLIT(L[2]);
	RANGE(icn);
	JMP(L[1]);
	LABEL(L[2]); RET(1);
}

//endl- : '\n';
__private void def_endl(recom_s* rc){
	token_char(rc, "endl", "\n", 0);
}

//sep-  : '[ \t]+';
__private void def_sep(recom_s* rc){
	token_class(rc, "sep", " \t", 0, 0);
}

//comma-: ';';
__private void def_comma(recom_s* rc){
	token_char(rc, "comma", ";", 0);
}

//colon-: ':';
__private void def_colon(recom_s* rc){
	token_char(rc, "colon", ":", 0);
}

//dot-  : '.';
__private void def_dot(recom_s* rc){
	token_char(rc, "dot", ".", 0);
}

//any-  : '[^\n]*';
__private void def_any(recom_s* rc){
	token_class(rc, "any", "\n", 1, 0);
}

//fn    : 'fn';
__private void def_fn(recom_s* rc){
	token_char(rc, "fn", "fn", 1);
}

//dec   : 'dec';
__private void def_dec(recom_s* rc){
	token_char(rc, "dec", "dec", 1);
}

//range : 'range';
__private void def_range(recom_s* rc){
	token_char(rc, "range", "range", 1);
}

//escape: '\\[nt]';
__private void def_escape(recom_s* rc){
	INIT(rc);
	FN("escape", 1);
	USERANGE();
	RRANGE();
	RRSTR("[nt]", 0);
	unsigned is = RRADD();
	CHAR('\\');
	RANGE(is);
	RET(1);
}

//nscape: '[^\n\t];
__private void def_nescape(recom_s* rc){
	INIT(rc);
	FN("nescape", 1);
	USERANGE();
	RRANGE();
	RRSTR("[\n\t]", 1);
	unsigned is = RRADD();
	RANGE(is);
	RET(1);
}

//ch    : escape
//      | num
//      | nescape
//      ;
__private void def_ch(recom_s* rc){
	INIT(rc);
	FN("ch", 1);
	USELBL(3);
	SPLIT(L[1]);
	CALL("escape");
	JMP(L[0]);
	LABEL(L[1]); SPLIT(L[2]);
	CALL("num");
	JMP(L[0]);
	LABEL(L[2]); CALL("nescape");
	LABEL(L[0]); RET(1);
}

//char  : '\'' nescape '\''
//      | '\'' escape '\''
//      ;
__private void def_char(recom_s* rc){
	INIT(rc);
	FN("char", 1);
	USELBL(2);
	CHAR('\'');
	SPLIT(L[1]);
	CALL("escape");
	JMP(L[0]);
	LABEL(L[1]); CALL("nescape");
	LABEL(L[0]); CHAR('\'');
	RET(1);
}

//emptyline-: sep? endl;
__private void def_emptyline(recom_s* rc){
	INIT(rc);
	FN("emptyline", 0);
	USELBL(1);
	SPLIT(L[0]);
	CALL("sep");
	LABEL(L[0]); CALL("endl");
	RET(0);
}

//comment-  : comma any;
__private void def_comment(recom_s* rc){
	INIT(rc);
	FN("comment", 0);
	CALL("comma");
	CALL("any");
	RET(0);
}

//label     : word sep? colon;
__private void def_label(recom_s* rc){
	INIT(rc);
	FN("label", 1);
	USELBL(1);
	CALL("word");
	SPLIT(L[0]);
	CALL("sep");
	LABEL(L[0]); CALL("colon");
	RET(1);
}

//decfn     : fn dot word sep? colon;
__private void def_decfn(recom_s* rc){
	INIT(rc);
	FN("decfn", 1);
	USELBL(1);
	CALL("fn");
	CALL("dot");
	CALL("word");
	SPLIT(L[0]);
	CALL("sep");
	LABEL(L[0]); CALL("colon");
	RET(1);
}

//endcmd    : sep? comment
//          | sep? endl
//          ;
__private void def_endcmd(recom_s* rc){
	INIT(rc);
	FN("endcmd", 1);
	USELBL(3);
	SPLIT(L[0]);
	CALL("sep");
	LABEL(L[0]); SPLIT(L[1]);
	CALL("comment");
	JMP(L[2]);
	LABEL(L[1]); CALL("endl");
	LABEL(L[2]); RET(1);
}

//chrange   : ch ('-' ch)?;
__private void def_chrange(recom_s* rc){
	INIT(rc);
	FN("chrange", 1);
	USELBL(1);
	CALL("ch");
	SPLIT(L[0]);
	CHAR('-');
	CALL("ch");
	LABEL(L[0]); RET(1);
}

//chclass   : '\'' chrange+ '\'';
__private void def_chclass(recom_s* rc){
	INIT(rc);
	FN("chclass", 1);
	USELBL(1);
	CHAR('\'');
	LABEL(L[0]); CALL("chrange");
	SPLIR(L[0]);
	CHAR('\'');
	RET(1);
}

//decrange  : dec dot range '[' num ']' sep? colon sep? chclass endcmd
__private void def_decrange(recom_s* rc){
	INIT(rc);
	FN("decrange", 1);
	USELBL(2);
	CALL("dec");
	CALL("dot");
	CALL("range");
	CHAR('[');
	CALL("num");
	CHAR(']');
	SPLIT(L[0]);
	CALL("sep");
	LABEL(L[0]); CALL("colon");
	SPLIT(L[1]);
	CALL("sep");
	LABEL(L[1]); CALL("chclass");
	CALL("endcmd");
	RET(1);
}

//arg   : char
//      | num
//      | word
//      ;
__private void def_arg(recom_s* rc){
	INIT(rc);
	FN("arg", 1);
	USELBL(3);
	SPLIT(L[1]);
	CALL("char");
	JMP(L[0]);
	LABEL(L[1]); SPLIT(L[2]);
	CALL("num");
	JMP(L[0]);
	LABEL(L[2]); CALL("word");
	LABEL(L[0]); RET(1);
}

//cmdsym: word endcmd;
__private void def_cmdsym(recom_s* rc){
	INIT(rc);
	FN("cmdsym", 1);
	CALL("word");
	CALL("endcmd");
	RET(1);
}

//cmdarg: word sep arg endcmd;
__private void def_cmdarg(recom_s* rc){
	INIT(rc);
	FN("cmdarg", 1);
	CALL("word");
	CALL("sep");
	CALL("arg");
	CALL("endcmd");
	RET(1);
}

//cmd   : sep? cmdsym
//      | sep? cmdarg
//      ;
__private void def_cmd(recom_s* rc){
	INIT(rc);
	FN("cmd", 1);
	USELBL(3);
	SPLIT(L[2]);
	CALL("sep");
	LABEL(L[2]); SPLIT(L[1]);
	CALL("cmdsym");
	JMP(L[0]);
	LABEL(L[1]); CALL("cmdarg");
	LABEL(L[0]); RET(1);
}

//cmdlbl: label cmd;
__private void def_cmdlbl(recom_s* rc){
	INIT(rc);
	FN("cmdlbl", 1);
	CALL("label");
	CALL("cmd");
	RET(1);
}

//cmdfn : decfn cmd;
__private void def_cmdfn(recom_s* rc){
	INIT(rc);
	FN("cmdfn", 1);
	CALL("decfn");
	CALL("cmd");
	RET(1);
}

//command: sep? cmdfn
//       | sep? cmdlbl
//       | cmd
//       | sep? decrange
//       ;

__private void def_command(recom_s* rc){
	INIT(rc);
	FN("command", 1);
	USELBL(5);
	SPLIT(L[0]);
	CALL("sep");
	LABEL(L[0]); SPLIT(L[2]);
	CALL("cmdfn");
	JMP(L[1]);
	LABEL(L[2]); SPLIT(L[3]);
	CALL("cmdlbl");
	JMP(L[1]);
	LABEL(L[3]); SPLIT(L[4]);
	CALL("cmd");
	JMP(L[1]);
	LABEL(L[4]); CALL("decrange");
	LABEL(L[1]); RET(1);
}

//progline-: emptyline
//         | sep? comment
//         | command
//         ;
__private void def_progline(recom_s* rc){
	INIT(rc);
	FN("progline", 0);
	USELBL(4);
	SPLIT(L[1]);
	CALL("emptyline");
	JMP(L[0]);
	LABEL(L[1]); SPLIT(L[2]);
	SPLIT(L[3]);
	CALL("sep");
	LABEL(L[3]); CALL("comment");
	JMP(L[0]);
	LABEL(L[2]); CALL("command");
	LABEL(L[0]); RET(0);
}

//program: progline+;
__private void def_program(recom_s* rc){
	INIT(rc);
	FN("program", 1);
	USELBL(1);
	LABEL(L[0]); CALL("progline");
	SPLIR(L[0]);
	RET(1);
}

//_start_: program;
uint16_t* reasmgram_make(void){
	uint16_t* ret = NULL;
	CTOR();
	INIT(ROBJ());
	def_num(ROBJ());
	def_word(ROBJ());
	def_endl(ROBJ());
	def_sep(ROBJ());
	def_comma(ROBJ());
	def_colon(ROBJ());
	def_dot(ROBJ());
	def_any(ROBJ());
	def_fn(ROBJ());
	def_dec(ROBJ());
	def_range(ROBJ());
	def_escape(ROBJ());
	def_nescape(ROBJ());
	def_ch(ROBJ());
	def_char(ROBJ());
	def_emptyline(ROBJ());
	def_comment(ROBJ());
	def_label(ROBJ());
	def_decfn(ROBJ());
	def_endcmd(ROBJ());
	def_chrange(ROBJ());
	def_chclass(ROBJ());
	def_decrange(ROBJ());
	def_arg(ROBJ());
	def_cmdsym(ROBJ());
	def_cmdarg(ROBJ());
	def_cmd(ROBJ());
	def_cmdlbl(ROBJ());
	def_cmdfn(ROBJ());
	def_command(ROBJ());
	def_progline(ROBJ());
	def_program(ROBJ());
	START(0);
	CALL("program");
	MATCH();
	ret = MAKE();
	DTOR();
	return ret;
}







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
	FN(name, 1);
	switch( quantifier ){
		case 0: RANGE(r); break;
		case 1:{ //+
			USELBL(1);
			LABEL(L[0]); RANGE(r);
			SPLIR(L[0]);
		}
		break;
	}
	RET(1);
}

__private void token_rename(recom_s* rc, const char* oldname, const char* newname){
	INIT(rc);
	FN(newname, 1);
	CALL(oldname, 0);
	RET(1);
}

//opchar : '[|*+?(){}[]];
__private void def_opchar(recom_s* rc){
	token_class(rc, "opchar", "|*+?(){}[]", 0, 0);
}

//literal: '[^|*+?(){}[].];
__private void def_literal(recom_s* rc){
	token_class(rc, "literal", "|*+?(){}[]", 1, 0);
}

//escaped: '\[|*+?(){}[].\^$]';
__private void def_escaped(recom_s* rc){
	INIT(rc);
	unsigned r = token_range(rc, "|*+?(){}[].^$", 0);
	FN("escaped", 1);
	CHAR('\\');
	RANGE(r);
	RET(1);
}

//char   : escaped
//       | literal
//;
__private void def_char(recom_s* rc){
	INIT(rc);
	USELBL(2);
	FN("char", 1);
	SPLIT(L[0]);
	CALL("escaped", 0);
	JMP(L[1]);
	LABEL(L[0]); CALL("literal",0);
	LABEL(L[1]); RET(1);
}

//number : [0-9]+;
__private void def_number(recom_s* rc){
	token_class(rc, "number", "0123456789", 0, 1);
}

//begin  : '^';
__private void def_begin(recom_s* rc){
	INIT(rc);
	FN("begin", 1);
	CHAR('^');
	RET(1);
}

//end    : '$';
__private void def_end(recom_s* rc){
	INIT(rc);
	FN("end", 1);
	CHAR('$');
	RET(1);
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
	FN("qspec", 1);
	CHAR('{');
	CALL("lnum", 0);
	SPLIT(L[0]);
	CHAR(',');
	SPLIT(L[0]);
	CALL("rnum", 0);
	LABEL(L[0]); CHAR('}');
	RET(1);
}

//quanti : qtype
//       | qspec
//;
__private void def_quanti(recom_s* rc){
	INIT(rc);
	USELBL(2);
	FN("quanti", 1);
	SPLIT(L[0]);
	CALL("qtype", 0);
	JMP(L[1]);
	LABEL(L[0]); CALL("qspec", 0);
	LABEL(L[1]); RET(1);
}

//chrange: char ('-' char)?;
__private void def_chrange(recom_s* rc){
	INIT(rc);
	USELBL(1);
	FN("chrange", 1);
	CALL("char", 0);
	SPLIT(L[0]);
	CHAR('-');
	CALL("char", 0);
	LABEL(L[0]); RET(1);
}

//chclass: '[' chrange+ ']';
__private void def_chclass(recom_s* rc){
	INIT(rc);
	USELBL(1);
	FN("chclass", 1);
	CHAR('[');
	LABEL(L[0]); CALL("chrange", 0);
	SPLIR(L[0]);
	CHAR(']');
	RET(1);
}

//group  : '(' altern ')'
__private void def_group(recom_s* rc){
	INIT(rc);
	FN("group", 1);
	CHAR('(');
	CALL("altern", 0);
	CHAR(')');
	RET(1);
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
	FN("primary", 1);
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
	LABEL(L[0]); RET(1);
}

//repet  : primary quanti?;
__private void def_repet(recom_s* rc){
	INIT(rc);
	USELBL(1);
	FN("repet", 1);
	CALL("primary", 0);
	SPLIT(L[0]);
	CALL("quanti", 0);
	LABEL(L[0]); RET( 1);
}

//concat : repet+;
__private void def_concat(recom_s* rc){
	INIT(rc);
	USELBL(1);
	FN("concat", 1);
	LABEL(L[0]); CALL("repet", 0);
	SPLIR(L[0]); RET( 1);
}

//altern : concat ( '|' concat )*;
__private void def_altern(recom_s* rc){
	INIT(rc);
	USELBL(2);
	FN("altern", 1);
	LABEL(L[1]); CALL("concat", 0);
	SPLIT(L[0]);
	CHAR('|');
	CALL("concat", 0);
	JMP(L[1]);
	LABEL(L[0]); RET( 1);
}

//regex  : '/' begin? altern end?'/'
__private void def_regex(recom_s* rc){
	INIT(rc);
	USELBL(2);
	FN("regex", 1);
	CHAR('/');
	SPLIT(L[0]);
	CALL("begin", 0);
	LABEL(L[0]); CALL("altern", 0);	
	SPLIT(L[1]);
	CALL("end", 0);
	LABEL(L[1]); CHAR('/');
	RET( 1);
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
	CALL("regex",0);
	ret = MAKE();
	DTOR();
	return ret;
}

uint16_t* regram_test(void){
	uint16_t* ret = NULL;
	CTOR();
	INIT(ROBJ());
	USELBL(2);
	USERANGE();
	RRANGE();
	RRSET('"');
	RRREV();
	unsigned nv = RRADD();
	
	FN("onlych",1);
		CHAR('s');
	RET(1);
	
	FN("grabquoted",1);
		CHAR('"');
		SAVE(2);
		LABEL(L[0]); SPLIT(L[1]);
		RANGE(nv);
		JMP(L[0]);
		LABEL(L[1]); SAVE(3);
		CHAR('"');
	RET(1);
	
	START(0);
	CALL("onlych", 0);
	CALL("grabquoted", 0);
	MATCH();
	ret = MAKE();
	DTOR();
	return ret;
}
*/






















