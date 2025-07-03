#include <notstd/utf8.h>
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

/^/ che inizia con : settare flags BEGIN con o senza MULTILINE
/$/ che finisce con: basta mettere char 0 prima di match o char \n se multiline
//G set vmgreedy otherwise classic thomson, lazy

//header
[0] 0xEE1A
[1] flags
[2] range count
[3] unicode count
[4] start
[5] bytecode len
//.section range
[16 bytecode per range] * rengecount
//.section urange
[16 bytecode ascii]
[512 bytecode range]: <count><empty><empty<empty> <2 bytecode start><2 bytecode end>

/[a-z]/

256/8
16

TODO
- //g per lavorare globalmente con funzione esterna tipo revm_continue
- test
- creare comandi per creare un AST in maniera automatica tipo con CMD_NODE e forse CMD_TOKEN

- creare grammatica
	dalla grammatica fare un lexer/parser direttamente in bytecode per implementare un compilatore reasm
*/

#define MAX_CALL    256

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

__private void pc_bitmap_reset(revm_s* vm, uint16_t* bytecode){
	memset(vm->pcBitmap, 0, bytecode[BYC_LEN]);
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

//very wrong implementation, ultra slow
__private int cmd_urange(const uint16_t* bc,  unsigned idr, ucs4_t ch){
	const unsigned offset = (BYC_SECTION_RANGE + bc[BYC_RANGE_COUNT] * 16) + (16+512)*idr;
	bc = &bc[offset];
	if( ch <= 128 ){
		const unsigned im = ch & 0x0F;
		const unsigned bm = 1 << (ch>>4);
		return bc[im] & bm ? 1 : 0;
	}
	const unsigned count = bc[16];
	bc += 4;
	for( unsigned i = 0; i < count; ++i ){
		ucs4_t rec = ((ucs4_t)bc[0]<<16) | bc[1];
		if( ch < rec ) return 0;
		rec = ((ucs4_t)bc[2]<<16) | bc[3];
		if( ch < rec ) return 1;
		bc += 4;
	}
	return 0;
}

revmMatch_s revm_match(const utf8_t* txt, const uint16_t* bytecode){
	revmMatch_s ret = {
		.match = 0,
		.capture[0] = txt,
		.capture[1] = txt
	};

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
	const utf8_t* begin = txt;
	const unsigned search    = bytecode[BYC_FLAGS] & BYTECODE_FLAG_BEGIN ? 0 : 1;
	const unsigned multiline = bytecode[BYC_FLAGS] & BYTECODE_FLAG_MULTILINE;
	const unsigned greedy    = bytecode[BYC_FLAGS] & BYTECODE_FLAG_GREEDY;

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
					if( greedy ){
						if( vm.allThreads[tid].save[1] - vm.allThreads[tid].save[0] > ret.capture[1] - ret.capture[0] ){
							memcpy(ret.capture, vm.allThreads[tid].save, (vm.allThreads[tid].lastsave+1) * sizeof(uint8_t*));
							ret.match = 1;
						}
						break;
					}
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
					if( !cmd_range(bytecode, BYTECODE_CMD412(byc), *txt) ){
						vmthr_release(&vm, tid);
					}
					else if( vmthr_idle(&vm, tid, pc+1) ){
						ret.match = -2;
						return ret;
					}
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
		
		if( vm.runr == vm.runw ){
			if( search ){
				if( multiline ){
					if( *begin != '\n' ) begin = (const utf8_t*)strchrnul((const char*)begin, '\n');
					if( *begin ) ++begin;
				}
				else{
					begin = utf8_codepoint_next(begin);
				}
				txt = begin;
				unsigned tid = vmthr_new(&vm);
				vm.allThreads[tid].pc = 0;
				vm.runThreads[FAST_MOD_POW_TWO(vm.runw, vm.max)] = tid;
				++vm.runw;
			}
			else{
				break;
			}
		}
		else{
			txt = utf8_codepoint_next(txt);
		}
		
	}//loop txt
	
	return ret;
}



/*

regex vm stack based


000000000 00000000 00000000 00000000



*/


typedef struct revmStack{
	unsigned      pc;
	const utf8_t* txt;
	unsigned      ssp;
	unsigned      csp;
}revmStack_s;

typedef struct revmCapture{
	const utf8_t* start;
	const utf8_t* end;
}revmCapture_s;

typedef struct revm2{
	uint16_t*      bytecode;
	uint16_t*      fn;
	uint16_t*      range;
	revmStack_s*   stack;
	revmCapture_s* capture;
	unsigned*      cstk;
	const utf8_t*  txt;
	uint32_t       start;
	uint32_t       pc;
}revm2_s;

__private void stk_push(revm2_s* vm, unsigned pc, const utf8_t* txt){
	unsigned i = m_ipush(&vm->stack);
	vm->stack[i].pc  = pc;
	vm->stack[i].txt = txt;
	vm->stack[i].ssp = m_header(vm->capture)->len;
	vm->stack[i].csp = m_header(vm->cstk)->len;
}

__private int stk_pop(revm2_s* vm){
	long i = m_ipop(vm->stack);
	if( i < 0 ) return 0;
	vm->pc  = vm->stack[i].pc;
	vm->txt = vm->stack[i].txt;
	m_header(vm->capture)->len = vm->stack[i].ssp;
	m_header(vm->cstk)->len = vm->stack[i].csp;
	return 1;
}

__private int range(revm2_s* vm, unsigned ir, utf8_t ch){
	const unsigned im = ch & 0x0F;
	const unsigned bm = 1 << (ch>>4);
	return vm->range[ir+im] & bm;
}

__private void capture_begin(revm2_s* vm){
	unsigned i = m_ipush(&vm->capture);
	vm->capture[i].start = vm->txt;
	vm->capture[i].end   = NULL;
}

__private void capture_end(revm2_s* vm){
	unsigned i = m_header(vm->capture)->len;
	iassert(i);
	iassert(vm->capture[i].end == NULL);
	vm->capture[i].end = vm->txt;
}

__private int capture_check(revm2_s* vm){
	mforeach(vm->capture,i){
		if( !vm->capture[i].end ) return 0;
	}
	return 1;
}

__private void call(revm2_s* vm, unsigned ifn){
	stk_push(vm, vm->start + vm->fn[ifn], vm->txt);
	unsigned i = m_ipush(&vm->cstk);
	vm->cstk[i] = vm->pc+1;
}

__private void ret(revm2_s* vm){
	long i = m_ipop(vm->cstk);
	iassert(i >= 0);
	stk_push(vm, vm->cstk[i], vm->txt);
}

__private void revm2_dtor(revm2_s* vm){
	m_free(vm->capture);
	m_free(vm->stack);
	m_free(vm->cstk);
}

revmCapture_s* revm_match2(uint16_t* bytecode, const utf8_t* txt){
	revm2_s vm;
	vm.bytecode = bytecode;
	vm.fn       = &bytecode[bytecode[BYC_SECTION_FN]];
	vm.range    = &bytecode[bytecode[BYC_SECTION_RANGE]];
	vm.pc       = 0;
	vm.stack    = MANY(revmStack_s, bytecode[BYC_LEN]);
	vm.capture  = MANY(revmCapture_s, 64);
	vm.cstk     = MANY(unsigned, 128);
	vm.start    = bytecode[BYC_START];

	stk_push(&vm, vm.start, txt);
	
	while( stk_pop(&vm) ){
		const unsigned byc = bytecode[vm.pc];
		const unsigned cmd = BYTECODE_CMD412(byc);
		switch( cmd ){
			case OP_MATCH:
				iassert(capture_check(&vm));
				revm2_dtor(&vm);
			return m_borrowed(vm.capture);
			
			case OP_CHAR:
				if( *txt == BYTECODE_VAL48(byc) ) stk_push(&vm, vm.pc+1, txt+1);
			break;
			
			case OP_RANGE:
				if( range(&vm, BYTECODE_VAL412(byc), *vm.txt) ) stk_push(&vm, vm.pc+1, txt+1);
			break;
			
			case OP_URANGE:
				//TODO
			break;
			
			case OP_SPLITF:
				stk_push(&vm, vm.pc + BYTECODE_VAL412(byc), txt);
				stk_push(&vm, vm.pc+1, txt);
			break;
			
			case OP_SPLITB:
				stk_push(&vm, vm.pc - BYTECODE_VAL412(byc), txt);
				stk_push(&vm, vm.pc+1, txt);
			break;
			
			case OP_SPLIRF:
				stk_push(&vm, vm.pc+1, txt);
				stk_push(&vm, vm.pc + BYTECODE_VAL412(byc), txt);
			break;
			
			case OP_SPLIRB:
				stk_push(&vm, vm.pc+1, txt);
				stk_push(&vm, vm.pc - BYTECODE_VAL412(byc), txt);
			break;
			
			case OP_JMPF:
				stk_push(&vm, vm.pc + BYTECODE_VAL412(byc), txt);
			break;
			
			case OP_JMPB:
				stk_push(&vm, vm.pc - BYTECODE_VAL412(byc), txt);
			break;
			
			case OP_SAVEB:
				capture_begin(&vm);
			break;
			
			case OP_SAVEE:
				capture_end(&vm);
			break;
			
			case OP_CALL:
				call(&vm, BYTECODE_CMD412(byc));
			break;

			case OP_RET:
				ret(&vm);
			break;
		}
	}
	
	revm2_dtor(&vm);
	return NULL;
}

































