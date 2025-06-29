#include "notstd/utf8.h"
#include <notstd/core.h>
#include <notstd/str.h>
#include <notstd/memory.h>
#include <mfmrevm.h>

/*

char   ch   ; if *str == ch
range  id   ; check range id
urange id   ; check range fo unicode
split  label; continue to pc+1 and start thread label
splir  label; continue to label and start thread pc+1
jmp    label; set pc to label
save   id   ; capture with id as name of capture
call   label; call function label
ret        ; return from call
node   id   ; create node with id
token  id   ; last save is token id

/e/
    char  e

/[a-z]/
    range 0

/e1|e2/
    split L1
    #code e1
    jmp   L2
L1: #code E2
L2: #continue

#greedy
/e?/
    split L2
    #code E
L2: #code after e

#non greedy
/e? ?/
    splir L2
    #code E
L2: #code after e

/e* /
L1: split L2
    #code e
    jmp L1
L2: #continue after e

/e*?/
L1: splir L2
    #code e
    jmp L1
L2: #continue after e

/e+/
L1: #code e
    splir L1
    #continue after e

/e+?/
L1: #code e
    split L1
    #continue after e




/
NUM : [0-9]+
OP  : [/+*-]
EXPR: NUM OP NUM
PROG: EXPR
/

#asm
NUM : save
L1  : range 0
      splir L1
      token 0
      ret
OP  : save
      range 1
      token 1
      ret
EXPR: node 1
      call NUM
      call OP
      call NUM
      ret
PROG: node 0
      call EXPR
      match

/
expression     ::= term (( "+" | "-" ) term)*
term           ::= factor (( "*" | "/" ) factor)*
factor         ::= unary ( "!" )*
unary          ::= ("+" | "-")? primary
primary        ::= NUMBER | "(" expression ")"
/

expression:     call term
            IF: split FI
                split C1
                char '+'
                jmp   CB
            C1: char '-'
            CB: call term
                jmp IF
            FI: ret

term:     call factor
      IF: split FI
          split C1
          char  '*'
          jmp   CB
      C1: char '/'
      CB: call factor
          jmp IF
      FI: ret

factor:     call unary
        P0: split P1
            char '!'
            jmp P0
            ret

unary:     split CB
           split C1
           char '+'
           jmp  CB
       C1: char '-'
       CB: call primary

primary:     split C1
             call number
             jmp CB
         C1: char '('
             call expression
             char ')'
         CB: ret

number: range 0
        ret

program: call expression
         match


grammar Expr;

parse       : expression EOF ;

expression  : expression '+' term   # Add
            | expression '-' term   # Sub
            | term                  # ToTerm
            ;

term        : term '*' factor       # Mul
            | term '/' factor       # Div
            | factor                # ToFactor
            ;

factor      : unary                 # ToUnary ;

unary       : '!' unary             # Not
            | '-' unary             # Neg
            | '+' unary             # Pos
            | primary               # ToPrimary
            ;

primary     : NUMBER                # Num
            | BOOL                  # Bool
            | '(' expression ')'    # Parens
            ;

NUMBER      : [0-9]+ ('.' [0-9]+)? ;
BOOL        : 'true' | 'false' ;
WS          : [ \t\r\n]+ -> skip ;

{
	grammar:[{
			name : opadd,
			regex: '+'
		},{
			name: 'opdiff,
			regex: '-'
		},{
			name: 'add',
			eval: [ 'epression', 'opadd', 'term' ]
		},{
			name: 'add',
			eval: [ 'epression', 'opdiff', 'term' ]
		},{
			name: 'expression',
			choose:[ {eval:['add']}, {eval:['sub']}, {eval:['term']} ]
		},{
			name: 'start',
			eval: ['expression']
		}
	]
}

bytecode:
4bit cmd 12bit value
special cmd
0xF 4bit cmd 8 bit value

//header
[0] 0xEE1A
[1] range count
[2] unicode count
[3] start
[4] bytecode len
//.section range
[16 bytecode per range] * rengecount
[256] * unicode count

/[a-z]/

256/8
16

*/

#define BYTECODE_FORMAT   0xEE1A
#define BYC_FORMAT        0
#define BYC_RANGE_COUNT   1
#define BYC_URANGE_COUNT  2
#define BYC_START         3
#define BYC_LEN           4
#define BYC_SECTION_RANGE 5

#define BYTECODE_CMD412(BYTECODE) ((BYTECODE)&0xF000)
#define BYTECODE_VAL412(BYTECODE) ((BYTECODE)&0x0FFF)

typedef enum{
	CMD_MATCH  = 0x0000,
	CMD_CHAR   = 0x1000,
	CMD_UNICODE= 0x2000,
	CMD_RANGE  = 0x3000,
	CMD_URANGE = 0x4000,
	CMD_SPLIT  = 0x5000,
	CMD_SPLIR  = 0x6000,
	CMD_JMP    = 0x7000,
	CMD_SAVE   = 0x8000,
	CMD_CALL   = 0x9000,
	CMD_RET    = 0xA000
}revmCmd_e;

typedef struct revmThr{
	unsigned       pc;
	const utf8_t** save;
	unsigned*      cstk;
}revmThr_s;

__private void thr_dtor(void* p){
	revmThr_s* t = *(void**)p;
	m_free(t->cstk);
	m_free(t->save);
}

void vmthr_add(uint8_t* pcbm, unsigned byclen, revmThr_s** thr, long tid, unsigned pc){
	unsigned il = pc >> 3;
	unsigned sb = 1<<(pc & 0x03);
	if( pcbm[il] & sb ) return;
	pcbm[il] |= sb;
	unsigned id = m_ipush(&thr);
	iassert( id < 4096 );
	(*thr)[id].pc   = pc;
	(*thr)[id].cstk = MANY(unsigned, byclen/4);
	(*thr)[id].save = MANY(const utf8_t*, byclen/4);
	if( tid >= 0 ){
		mforeach((*thr)[tid].cstk, it){
			unsigned is = m_ipush(&(*thr)[id].cstk);
			(*thr)[id].cstk[is] = (*thr)[tid].cstk[it];
		}
		mforeach((*thr)[tid].save, it){
			unsigned is = m_ipush(&(*thr)[id].save);
			(*thr)[id].save[is] = (*thr)[tid].save[it];
		}
	}
}

__private void cmd_save(revmThr_s* t, const utf8_t* txt, unsigned id){
	if( id > m_header(t->save)->count ){
		t->save = m_realloc(t->save, id*2);
	}
	t->save[id] = txt;
	if( id + 1 > m_header(t->save)->len ){
		m_header(t->save)->len = id;
	}
}

__private int cmd_range(const uint16_t* bc, unsigned idr, utf8_t ch){
	idr = idr*16 + BYC_SECTION_RANGE;
	const unsigned im = ch & 0x0F;
	const unsigned bm = 1 << (ch>>4);
	return bc[idr+im] & bm ? 1 : 0;
}

revmMatch_s revm_run(const utf8_t* txt, const uint16_t* bytecode){
	revmMatch_s ret;
	if( bytecode[0] != BYTECODE_FORMAT ) {
		ret.match = -1;
		return ret;
	}
	
	__free revmThr_s* cthr = MANY(revmThr_s, bytecode[BYC_LEN], thr_dtor);
	__free revmThr_s* nthr = MANY(revmThr_s, bytecode[BYC_LEN], thr_dtor);
	__free uint8_t*   pcbm = MANY(uint8_t, (bytecode[BYC_LEN]/8)+1);
	m_zero(pcbm);
	
	const unsigned start = bytecode[BYC_START];
	vmthr_add(pcbm, bytecode[BYC_LEN], &cthr, -1, 0);
	while( *txt ){
		mforeach(cthr, it){
			const unsigned cmd = BYTECODE_CMD412(bytecode[start+cthr[it].pc]);
			const unsigned val = BYTECODE_VAL412(bytecode[start+cthr[it].pc]);
			switch( cmd ){
				case CMD_MATCH:
					cmd_save(&cthr[it], txt, 1);
					ret.capture = m_borrowed(cthr[it].save);
					ret.match = 1;
				return ret;
				
				case CMD_CHAR:
					if( *txt != val ) break;
					vmthr_add(pcbm, bytecode[BYC_LEN], &nthr, it, cthr[it].pc+1);
				break;
				
				case CMD_UNICODE:{
					ucs4_t u = utf8_to_ucs4(txt);
					ucs4_t m = (bytecode[start+cthr[it].pc+1] << 16) | (bytecode[start+cthr[it].pc+2]);
					if( u != m ) break;
					vmthr_add(pcbm, bytecode[BYC_LEN], &nthr, it, cthr[it].pc+3);
				}
				break;
				
				case CMD_RANGE:
					if( !cmd_range(bytecode, val, *txt) ) break;
					vmthr_add(pcbm, bytecode[BYC_LEN], &nthr, it, cthr[it].pc+1);
				break;
				
				case CMD_URANGE:
				break;
				
				case CMD_SPLIT:
					vmthr_add(pcbm, bytecode[BYC_LEN], &cthr, it, cthr[it].pc+1);
					vmthr_add(pcbm, bytecode[BYC_LEN], &cthr, it, val);
				break;
				
				case CMD_SPLIR:
					vmthr_add(pcbm, bytecode[BYC_LEN], &cthr, it, val);
					vmthr_add(pcbm, bytecode[BYC_LEN], &cthr, it, cthr[it].pc+1);
				break;
				
				case CMD_JMP:
					vmthr_add(pcbm, bytecode[BYC_LEN], &cthr, it, val);
				break;
				
				case CMD_SAVE:
					cmd_save(&cthr[it], txt, val);
					vmthr_add(pcbm, bytecode[BYC_LEN], &cthr, it, val);
				break;
				
				case CMD_CALL:
					cthr[it].cstk[m_ipush(&cthr[it].cstk)] = cthr[it].pc+1;
					vmthr_add(pcbm, bytecode[BYC_LEN], &cthr, it, val);
				break;
				
				case CMD_RET:
					vmthr_add(pcbm, bytecode[BYC_LEN], &cthr, it, cthr[it].cstk[m_ipop(cthr[it].cstk)]);
				break;
			}//switch cmd
			
			m_free(cthr[it].save);
			m_free(cthr[it].cstk);
		}//loop thread

		m_clear(cthr);
		swap(cthr, nthr);
		txt = utf8_codepoint_next(txt);
	}//loop txt
	
	ret.match   = 0;
	ret.capture = NULL;
	return ret;
}
















