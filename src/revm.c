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

/ab[a-z]/


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

TODO
- unicode range
- //m per lavorare in multilinea
- //g per lavorare globalmente con funzione esterna tipo revm_continue
- /^/ che inizia con
- /$/ che finisce con
- test
- creare comandi per creare un AST in maniera automatica tipo con CMD_NODE e forse CMD_TOKEN

- creare grammatica
	dalla grammatica fare un lexer/parser direttamente in bytecode per implementare un compilatore reasm
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
#define BYTECODE_VAL48(BYTECODE) ((BYTECODE)&0x00FF)

#define MAX_CALL    256

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
	unsigned      pc;
	const utf8_t* save[REVM_MAX_CAPTURE*2];
	unsigned      lastsave;
	uint16_t      cstk[256];
	unsigned      csp;
}revmThr_s;

typedef struct revm{
	revmThr_s* allThreads;
	unsigned*  availableThreads;
	unsigned   max;
	unsigned*  runThreads;
	unsigned   runr;
	unsigned   runw;
	unsigned*  idleThreads;
	unsigned   idler;
	unsigned   idlew;
	uint8_t*   pcBitmap;
}revm_s;

__private unsigned vmthr_new(revm_s* vm){
	long tid = m_ipop(vm->availableThreads);
	if( tid == -1 ){
		dbg_info("end of threads, allocate more space");
		unsigned ntids = m_header(vm->allThreads)->count;
		vm->allThreads = m_grow(vm->allThreads, vm->max);
		m_header(vm->allThreads)->len = m_header(vm->allThreads)->count;
		for( unsigned i = 0; i < vm->max; ++i){
			vm->availableThreads[i] = ntids+i;
		}
		m_header(vm->availableThreads)->len = m_header(vm->availableThreads)->count;
		tid = m_ipop(vm->availableThreads);
		iassert(tid != -1);
	}
	vm->allThreads[tid].csp = 0;
	vm->allThreads[tid].lastsave = 0;
	return tid;
}

__private int pc_bitmap(uint8_t* pcbm, unsigned pc){
	unsigned il = pc >> 3;
	unsigned sb = 1<<(pc & 0x03);
	if( pcbm[il] & sb ) return 0;
	pcbm[il] |= sb;
	return 1;
}

__private int vmthr_clone(revm_s* vm, unsigned tid, unsigned newpc){
	if( !pc_bitmap(vm->pcBitmap, newpc) ) return 0;
	if( vm->runw+1 == vm->runr ) return -1;
	revmThr_s* t = &vm->allThreads[tid];
	unsigned ntid = vmthr_new(vm);
	memcpy(vm->allThreads[ntid].save, t->save, (t->lastsave+1) * sizeof(uint8_t*));
	vm->allThreads[ntid].lastsave = t->lastsave;
	if( t->csp ) memcpy(vm->allThreads[ntid].cstk, t->cstk, t->csp * sizeof(uint16_t));
	vm->allThreads[ntid].csp = t->csp;
	vm->allThreads[ntid].pc  = newpc;
	vm->runThreads[FAST_MOD_POW_TWO(vm->runw, vm->max)] = ntid;
	++vm->runw;
	return 0;
}

__private void vmthr_release(revm_s* vm, unsigned tid){
	vm->availableThreads[m_ipush(vm->availableThreads)] = tid;
}

__private int vmthr_idle(revm_s* vm, unsigned tid, unsigned pc){
	if( !pc_bitmap(vm->pcBitmap, pc) ){
		vmthr_release(vm, tid);
		return 0;
	}
	if( vm->idlew+1 == vm->idler ) return -1;
	vm->allThreads[tid].pc  = pc;
	vm->idleThreads[FAST_MOD_POW_TWO(vm->idlew, vm->max)] = tid;
	++vm->idlew;
	return 0;
}

__private int vmthr_continue(revm_s* vm, unsigned tid, unsigned pc){
	if( !pc_bitmap(vm->pcBitmap, pc) ){
		vmthr_release(vm, tid);
		return 0;
	}
	if( vm->runw+1 == vm->runr ) return -1;
	vm->allThreads[tid].pc  = pc;
	vm->runThreads[FAST_MOD_POW_TWO(vm->runw, vm->max)] = tid;
	++vm->runw;
	return 0;
}

__private void cmd_save(revm_s* vm, unsigned tid, unsigned id, const utf8_t* txt){
	if( id > vm->allThreads[tid].lastsave ) vm->allThreads[tid].lastsave = id;
	vm->allThreads[tid].save[id] = txt;
}

__private int cmd_range(const uint16_t* bc, unsigned idr, utf8_t ch){
	idr = idr*16 + BYC_SECTION_RANGE;
	const unsigned im = ch & 0x0F;
	const unsigned bm = 1 << (ch>>4);
	return bc[idr+im] & bm ? 1 : 0;
}

revmMatch_s revm_match(const utf8_t* txt, const uint16_t* bytecode){
	revmMatch_s ret;
	if( bytecode[0] != BYTECODE_FORMAT ) {
		ret.match = -1;
		return ret;
	}
	
	revm_s vm;
	vm.max = ROUND_UP_POW_TWO32(bytecode[BYC_LEN]);
	vm.runr = 0;
	vm.runw = 0;
	vm.idler = 0;
	vm.idlew = 0;
	vm.allThreads = MANY(revmThr_s, vm.max);
	vm.runThreads = MANY(unsigned, vm.max);
	vm.idleThreads = MANY(unsigned, vm.max);
	vm.pcBitmap = MANY(uint8_t, (bytecode[BYC_LEN]/8)+1);
	m_zero(vm.pcBitmap);
	vm.availableThreads = MANY(unsigned, vm.max);
	for( unsigned i = vm.max; i < vm.max; ++i ){
		vm.availableThreads[i] = i;
	}
	m_header(vm.availableThreads)->len = vm.max;
	const unsigned start = bytecode[BYC_START];

	unsigned tid = vmthr_new(&vm);
	vm.allThreads[tid].pc = 0;
	vm.runThreads[FAST_MOD_POW_TWO(vm.runw, vm.max)] = tid;
	++vm.runw;
	
	while( *txt ){
		while( vm.runr != vm.runw ){
			tid = vm.runThreads[FAST_MOD_POW_TWO(vm.runr, vm.max)];
			++vm.runr;
			const unsigned pc = vm.allThreads[tid].pc;
			const unsigned byc = bytecode[start+pc];
			const unsigned cmd = BYTECODE_CMD412(byc);
			switch( cmd ){
				case CMD_MATCH:
					cmd_save(&vm, tid, 1, txt);
					memcpy(ret.capture, vm.allThreads[tid].save, (vm.allThreads[tid].lastsave+1) * sizeof(uint8_t*));
					ret.match = 1;
				return ret;
				
				case CMD_CHAR:
					if( *txt != BYTECODE_VAL48(byc) ){
						vmthr_release(&vm, tid);
					}
					else if( vmthr_idle(&vm, tid, pc+1) ){
						ret.match = -2;
						return ret;
					}
				break;
				
				case CMD_UNICODE:{
					ucs4_t u = utf8_to_ucs4(txt);
					ucs4_t m = (bytecode[start+pc+1] << 16) | (bytecode[start+pc+2]);
					if( u != m ){
						vmthr_release(&vm, tid);
					}
					else if( vmthr_idle(&vm, tid, pc+1) ){
						ret.match = -2;
						return ret;
					}
				}
				break;
				
				case CMD_RANGE:
					if( !cmd_range(bytecode, BYTECODE_CMD412(byc), *txt) ){
						vmthr_release(&vm, tid);
					}
					else if( vmthr_idle(&vm, tid, pc+1) ){
						ret.match = -2;
						return ret;
					}
				break;
				
				case CMD_URANGE:
				break;
				
				case CMD_SPLIT:
					if( vmthr_continue(&vm, tid, pc+1) ){
						ret.match = -3;
						return ret;
					}
					if( vmthr_clone(&vm, tid, BYTECODE_CMD412(byc)) ){
						ret.match = -3;
						return ret;
					}
				break;
				
				case CMD_SPLIR:
					if( vmthr_clone(&vm, tid, BYTECODE_CMD412(byc)) ){
						ret.match = -3;
						return ret;
					}
					if( vmthr_continue(&vm, tid, pc+1) ){
						ret.match = -3;
						return ret;
					}
				break;
				
				case CMD_JMP:
					if( vmthr_continue(&vm, tid, BYTECODE_CMD412(byc)) ){
						ret.match = -3;
						return ret;
					}
				break;
				
				case CMD_SAVE:
					cmd_save(&vm, tid, BYTECODE_CMD412(byc), txt);
					if( vmthr_continue(&vm, tid, BYTECODE_CMD412(byc)) ){
						ret.match = -3;
						return ret;
					}
				break;
				
				case CMD_CALL:
					if( vm.allThreads[tid].csp >= MAX_CALL ){
						dbg_error("to many call");
						ret.match = -4;
						return ret;
					}
					vm.allThreads[tid].cstk[vm.allThreads[tid].csp++] = vm.allThreads[tid].pc + 1;
					if( vmthr_continue(&vm, tid, BYTECODE_CMD412(byc)) ){
						ret.match = -3;
						return ret;
					}
				break;
				
				case CMD_RET:
					if( vm.allThreads[tid].csp ){
						dbg_error("ret without call");
						ret.match = -5;
						return ret;
					}
					if( vmthr_continue(&vm, tid, vm.allThreads[tid].cstk[--vm.allThreads[tid].csp]) ){
						ret.match = -3;
						return ret;
					}
				break;
			}//switch cmd
		}//loop thread
		
		vm.runr = vm.idler;
		vm.runw = vm.idlew;
		vm.idler = 0;
		vm.idlew = 0;
		swap(vm.runThreads, vm.idleThreads);
		txt = utf8_codepoint_next(txt);
	}//loop txt
	
	ret.match   = 0;
	return ret;
}
















