#include <notstd/utf8.h>
#include <notstd/memory.h>
#include <mfmrevm.h>
#include <readline/readline.h>
#include <notstd/str.h>
#include <inutility.h>

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


how call work:
FN: SPLIT CF
	CHAR a
    char b
	RET 0
CF: RET -1

CALL FN; push pc+1
       ; push pc[FN]
MATCH

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

/*

regex vm stack based

//header
[00] 0xEE1A
[01] flags
[02] range count
[03] unicode count
[04] fn count
[05] name count
[06] start
[07] bytecode len
[08] addr section fn
[09] addr section range
[0A] addr section urange
[0B] addr section name



*/

typedef struct lipsStack{
	unsigned      pc;
	const utf8_t* sp;
	int           ls;
	unsigned      cp;
	unsigned      np;
}lipsStack_s;

typedef struct lips{
	uint16_t*      bytecode;
	uint16_t*      fn;
	uint16_t*      range;
	uint16_t*      code;
	lipsStack_s*   stack;
	lipsError_s*   errors;
	const utf8_t*  save[LIPS_MAX_CAPTURE*2];
	unsigned*      cstk;
	bcnode_s*      node;
	const utf8_t*  sp;
	uint32_t       pc;
	int            ls;
	uint32_t       bytecodeLen;
	uint32_t       sectionStart;
	uint32_t       sectionFn;
	uint32_t       sectionRange;
	uint32_t       sectionURange;
	uint32_t       sectionName;
	uint32_t       sectionCode;
}lips_s;

__private lips_s* lips_ctor(lips_s* vm, uint16_t* bytecode, const utf8_t* txt){
	vm->sectionStart  = bytecode[BYC_START];
	vm->sectionCode   = bytecode[BYC_SECTION_CODE];
	vm->sectionRange  = bytecode[BYC_SECTION_RANGE];
	vm->sectionURange = bytecode[BYC_SECTION_URANGE];
	vm->sectionFn     = bytecode[BYC_SECTION_FN];
	vm->bytecodeLen   = bytecode[BYC_CODELEN];
	vm->bytecode = bytecode;
	vm->fn       = &bytecode[vm->sectionFn];
	vm->range    = &bytecode[vm->sectionRange];
	vm->code     = &bytecode[vm->sectionCode];
	vm->pc       = 0;
	vm->sp       = txt;
	vm->stack    = MANY(lipsStack_s, vm->bytecodeLen);
	vm->cstk     = MANY(unsigned, 128);
	vm->node     = MANY(bcnode_s, 32);
	vm->errors   = MANY(lipsError_s, 16);
	vm->ls       = -1;
	memset(vm->save, 0, sizeof vm->save);
	return vm;
}

__private void lips_dtor(lips_s* vm){
	m_free(vm->stack);
	m_free(vm->cstk);
	m_free(vm->node);
	m_free(vm->errors);
}

__private void stk_push(lips_s* vm, unsigned pc, const utf8_t* sp){
	unsigned i = m_ipush(&vm->stack);
	vm->stack[i].pc = pc;
	vm->stack[i].sp = sp;
	vm->stack[i].ls = vm->ls;
	vm->stack[i].cp = m_header(vm->cstk)->len;
	vm->stack[i].np = m_header(vm->node)->len;
}

__private int stk_pop(lips_s* vm){
	long i = m_ipop(vm->stack);
	if( i < 0 ) return 0;
	vm->pc = vm->stack[i].pc;
	vm->sp = vm->stack[i].sp;
	vm->ls = vm->stack[i].ls;
	m_header(vm->cstk)->len = vm->stack[i].cp;
	m_header(vm->node)->len = vm->stack[i].np;
	return 1;
}

__private void stk_return_call_success(lips_s* vm, unsigned pos){
	iassert(pos>0);
	vm->stack[pos-1].sp = vm->sp;
	vm->stack[pos-1].ls = vm->ls;
	vm->stack[pos-1].np = m_header(vm->node)->len;
	m_header(vm->stack)->len = pos;
}

__private void stk_return_call_fail(lips_s* vm, unsigned pos){
	iassert(pos>0);
	m_header(vm->stack)->len = pos-1;
}

__private int range(lips_s* vm, unsigned ir, utf8_t ch){
	const unsigned im = ch & 0x0F;
	const unsigned bm = 1 << (ch>>4);
	return vm->range[ir*16+im] & bm;
}

__private void save(lips_s* vm, unsigned id){
	iassert(id < 256);
	vm->save[id] = vm->sp;
	if( (int)id > vm->ls ) vm->ls = id;
}

__private void call(lips_s* vm, unsigned ifn){
	stk_push(vm, vm->pc+1, vm->sp);
	unsigned i = m_ipush(&vm->cstk);
	vm->cstk[i] = m_header(vm->stack)->len;
	stk_push(vm, vm->fn[ifn], vm->sp);
}

__private void cret(lips_s* vm, int fail){
	long i = m_ipop(vm->cstk);
	if( i >=0 ){
		if( fail ){
			stk_return_call_fail(vm, vm->cstk[i]);
		}
		else{
			stk_return_call_success(vm, vm->cstk[i]);
		}
	}
}

__private void cerror(lips_s* vm, uint8_t num){
	unsigned i = m_ipush(&vm->errors);
	vm->errors[i].loc    = vm->sp;
	vm->errors[i].number = num;
	if( num & LIPS_ERROR_DIE ) m_header(vm->stack)->len = 0;
}

__private void node(lips_s* vm, nodeOP_e op, unsigned id){
	unsigned i = m_ipush(&vm->node);
	vm->node[i].sp = vm->sp;
	vm->node[i].id = id;
	vm->node[i].op = op;
}

__private int vm_exec(lips_s* vm, lipsMatch_s* ret){
	const unsigned byc = vm->code[vm->pc];
	switch( BYTECODE_CMD40(byc) ){
		case OP_MATCH:
			save(vm, 1);
			memcpy(ret->capture, vm->save, sizeof(const utf8_t*) * (vm->ls+1));
			ret->match = (vm->ls+1)/2;
			if( m_header(vm->node)->len ) ret->ast = lips_ast_make(vm->node);
		return 0;
		
		case OP_CHAR:
			if( *vm->sp == BYTECODE_VAL08(byc) ){
				stk_push(vm, vm->pc+1, vm->sp+1);
			}
			else{
				cerror(vm, LIPS_ERR_UNKNOW_SYMBOL);
			}
		break;
		
		case OP_RANGE:
			if( range(vm, BYTECODE_VAL12(byc), *vm->sp) ){
				stk_push(vm, vm->pc+1, vm->sp+1);
			}
			else{
				cerror(vm, LIPS_ERR_UNKNOW_SYMBOL);
			}
		break;
		
		case OP_URANGE:
			//TODO
		break;
		
		case OP_SPLITF:
			stk_push(vm, vm->pc + BYTECODE_VAL12(byc), vm->sp);
			stk_push(vm, vm->pc+1, vm->sp);
		break;
		
		case OP_SPLITB:
			stk_push(vm, vm->pc - BYTECODE_VAL12(byc), vm->sp);
			stk_push(vm, vm->pc+1, vm->sp);
		break;
		
		case OP_SPLIRF:
			stk_push(vm, vm->pc+1, vm->sp);
			stk_push(vm, vm->pc + BYTECODE_VAL12(byc), vm->sp);
		break;
		
		case OP_SPLIRB:
			stk_push(vm, vm->pc+1, vm->sp);
			stk_push(vm, vm->pc - BYTECODE_VAL12(byc), vm->sp);
		break;
		
		case OP_JMPF:
			stk_push(vm, vm->pc + BYTECODE_VAL12(byc), vm->sp);
		break;
		
		case OP_JMPB:
			stk_push(vm, vm->pc - BYTECODE_VAL12(byc), vm->sp);
		break;
		
		case OP_NODE:
			node(vm, NOP_NEW, BYTECODE_VAL12(byc));
			stk_push(vm, vm->pc+1, vm->sp);	
		break;
		
		case OP_CALL:
			call(vm, BYTECODE_VAL12(byc));
		break;
		
		case OP_EXT:
			switch(BYTECODE_CMD04(byc)){
				case OPE_SAVE:
					save(vm, BYTECODE_VAL08(byc));
					stk_push(vm, vm->pc+1, vm->sp);
				break;
				
				case OPE_RET:
					cret(vm, BYTECODE_VAL08(byc));
				break;
				
				case OPE_NODEEX:
					node(vm, BYTECODE_VAL08(byc), 0);
					stk_push(vm, vm->pc+1, vm->sp);
				break;
				
				case OPE_ERROR:
					cerror(vm, BYTECODE_VAL08(byc));
				break;
			}
		break;
	}
	return -1;
}

int lips_match(uint16_t* bytecode, const utf8_t* txt, lipsMatch_s* ret){
	ret->match = 0;
   	ret->ast   = NULL;
	ret->err   = NULL;
	
	lips_s vm;
	lips_ctor(&vm, bytecode, txt);
	stk_push(&vm, vm.sectionStart, vm.sp);
	while( stk_pop(&vm) && vm_exec(&vm, ret) );
	if( ret->match < 1 ) ret->err = m_borrowed(vm.errors);
	lips_dtor(&vm);
	return ret->match;
}

const char** lips_map_name(const uint16_t* bytecode){
	unsigned const count = bytecode[BYC_FN_COUNT];
	const uint16_t* n = &bytecode[bytecode[BYC_SECTION_NAME]];
	const char** ret = MANY(const char*, count);
	m_header(ret)->len = count;
	for( unsigned i = 0; i < count; ++i ){
		ret[i] = (const char*)n;
		unsigned len = strlen(ret[i])+1;
		len = ROUND_UP(len, 2);
		n += len/2;
	}
	return ret;
}


























__private void term_cls(void){
	fputs("\033[2J\033[H", stdout);
}

__private void term_gotoxy(unsigned x, unsigned y){
	printf("\033[%d;%dH", y+1, x+1);
}

__private void term_nexty(unsigned y){
	printf("\033[%dB", y);
}

__private void term_prevx(unsigned x){
	printf("\033[%dD", x);
}


__private void term_fcol(unsigned col){
	printf("\033[38;5;%um", col);
}

__private void term_bcol(unsigned col){
	printf("\033[48;5;%um", col);
}

__private void term_reset(void){
	printf("\033[m");
}

__private void term_bold(void){
	printf("\033[1m");
}

__private void term_box(unsigned i){
	//                            0    1    2    3    4    5    6    7    8
	static const char* box[] = { "┌", "┬", "┐", "┤", "┘", "┴", "└", "├", "┼"};
	fputs(box[i], stdout);
}

__private void term_hline(unsigned len){
	while( len --> 0 ) fputs("─", stdout);
}

__private void term_vline(unsigned len){
	while( len --> 0 ){
		fputs("│", stdout);
		term_nexty(1);
		term_prevx(1);
	}
}

__private void term_cline(unsigned len){
	while( len --> 0 ) putchar(' ');
}

/*
	start:

	stack     | callstack   node | capture
	1         | 1         |       | 1       
	2         | 2         |       | 2       
	3         | 3         |       | 3         
    -------------------------------------------
	command
	1
	2
	3
	4
	5
	
	>

*/

#define DBG_HEADER_H  1
#define DBG_STACKED_H 5
#define DBG_RANGE_H   1
#define DBG_CODE_H    20
#define DBG_PROMPT_H  1
#define DBG_INP_H     2
#define DBG_SCREEN_W  120
#define DBG_STACK_W   (DBG_SCREEN_W/4)
#define DBG_CSTACK_W  (DBG_SCREEN_W/4-1)
#define DBG_NODE_W    (DBG_SCREEN_W/4-1)
#define DBG_CAPTURE_W (DBG_SCREEN_W/4-1)

typedef enum { DCMD_SUSPEND, DCMD_STEP, DCMD_CONTINUE, DCMD_QUIT, DCMD_BREAKPOINT, DCMD_BREAKREMOVE } dbgcommand_e;

typedef struct lipsdbg{
	lips_s        vm;
	const utf8_t* source;
	const char**  nmap;
	uint32_t*     breakpoint;
	dbgcommand_e  run;
	unsigned      srclen;
	uint32_t      curstk;
	long          next[2];
}lipsdbg_s;

typedef void (*dbgaction_f)(lipsdbg_s*, char** args);

__private void putc_view_char(unsigned ch){
	fputs(cast_view_char(ch, 1), stdout);
}

__private void draw_cx_line(unsigned box){
	term_box(7); term_hline(DBG_STACK_W); term_box(box);
	term_hline(DBG_CSTACK_W); term_box(box);
	term_hline(DBG_NODE_W); term_box(box);
	term_hline(DBG_CAPTURE_W); term_box(3);
}

__private void draw_box(void){
	unsigned y = DBG_HEADER_H+2;
	term_box(0); term_hline(DBG_SCREEN_W); term_box(2);
	term_gotoxy(0, 1);
	term_vline(DBG_HEADER_H);
	draw_cx_line(1);
	term_gotoxy(DBG_SCREEN_W+1, 1); term_vline(DBG_HEADER_H);

	term_gotoxy(0, y); term_vline(DBG_STACKED_H); 
	draw_cx_line(5);
	unsigned x = DBG_STACK_W+1;
	term_gotoxy(x, y); term_vline(DBG_STACKED_H);
	x += DBG_CSTACK_W+1;
	term_gotoxy(x, y); term_vline(DBG_STACKED_H);
	x += DBG_NODE_W+1;
	term_gotoxy(x, y); term_vline(DBG_STACKED_H);
	
	term_gotoxy(DBG_SCREEN_W+1, y); term_vline(DBG_STACKED_H);
	y += DBG_STACKED_H+1;
	term_gotoxy(0, y); term_vline(DBG_RANGE_H);
	term_box(7); term_hline(DBG_SCREEN_W); term_box(3);
	term_gotoxy(DBG_SCREEN_W+1, y); term_vline(DBG_RANGE_H);
	y += DBG_RANGE_H+1;
	term_gotoxy(0, y); term_vline(DBG_CODE_H);
	term_box(7); term_hline(DBG_SCREEN_W); term_box(3);
	term_gotoxy(DBG_SCREEN_W+1, y); term_vline(DBG_CODE_H);
	term_gotoxy(0, y); term_vline(DBG_CODE_H);
	
	y += DBG_CODE_H+1;
	term_gotoxy(0, y); term_vline(DBG_INP_H);
	term_box(7); term_hline(DBG_SCREEN_W); term_box(3);
	term_gotoxy(DBG_SCREEN_W+1, y); term_vline(DBG_INP_H);
	
	y += DBG_INP_H+1;
	term_gotoxy(0, y); term_vline(DBG_PROMPT_H);
	term_box(6); term_hline(DBG_SCREEN_W); term_box(4);
	term_gotoxy(DBG_SCREEN_W+1, y); term_vline(DBG_PROMPT_H);
}

__private void draw_cls_rect(unsigned x, unsigned y, unsigned w, unsigned h){
	unsigned const end = y+h;
	for(; y < end; ++y ){
		term_gotoxy(x,y);
		term_cline(w);
	}
}

__private void draw_clear(void){
	term_cls();
	draw_box();
}

__private void draw_header(unsigned start, unsigned ranges, unsigned functions, unsigned stacksize){
	draw_cls_rect(1, 1, DBG_SCREEN_W, DBG_HEADER_H);
	term_gotoxy(1, 1);
	printf("start:%04X ranges:%u functions:%u stacksize:%u", start, ranges, functions, stacksize);
}

__private void draw_stack(lipsStack_s* base, const utf8_t* txt){
	unsigned y  = DBG_HEADER_H+1+DBG_STACKED_H;
	unsigned ny = DBG_STACKED_H;
	unsigned count = m_header(base)->len;
	unsigned sc = count > ny ? count - ny: 0;
	draw_cls_rect(1, DBG_HEADER_H+2, DBG_STACK_W, DBG_STACKED_H);
	while( ny-->0 && sc < count){
		term_gotoxy(1,y--);
		printf(" %% pc:%04X sp:%4lu ls:%3d",
			base[sc].pc,
			base[sc].sp-txt,
			base[sc].ls
		);
		++sc;
	}
}

__private void draw_cstack(lips_s* vm){
	unsigned y  = DBG_HEADER_H+1+DBG_STACKED_H;
	unsigned ny = DBG_STACKED_H;
	unsigned count = m_header(vm->cstk)->len;
	unsigned sc = count > ny ? count - ny: 0;
	unsigned x = DBG_STACK_W+2;
	draw_cls_rect(x, DBG_HEADER_H+2, DBG_CSTACK_W, DBG_STACKED_H);
	while( ny-->0 && sc < count){
		term_gotoxy(x, y--);
		printf(" <- pc:%4X", vm->cstk[sc]);
		++sc;
	}
}

__private void draw_node(lips_s* vm, const char** nmap, const utf8_t* txt){
	unsigned y  = DBG_HEADER_H+1+DBG_STACKED_H;
	unsigned ny = DBG_STACKED_H;
	unsigned count = m_header(vm->node)->len;
	unsigned sc = count > ny ? count - ny: 0;
	unsigned x = DBG_STACK_W+2+DBG_CSTACK_W+1;
	draw_cls_rect(x, DBG_HEADER_H+2, DBG_NODE_W, DBG_STACKED_H);
	while( ny-->0 && sc < count){
		term_gotoxy(x, y--);
		switch( vm->node[sc].op ){
			case NOP_NEW    : printf(" + [%4u](%3d)%s %lu", sc, vm->node[sc].id, nmap[vm->node[sc].id], vm->node[sc].sp - txt); break;
			case NOP_PARENT : printf(" < [%4d] %lu", sc, vm->node[sc].sp - txt); break;
			case NOP_DISABLE: printf(" ~ [%4u]", sc); break;
			case NOP_ENABLE : printf(" & [%4u]", sc); break;
		}
		++sc;
	}
}

__private void draw_save(lips_s* vm, const utf8_t* txt){
	unsigned y  = DBG_HEADER_H+1+DBG_STACKED_H;
	unsigned ny = DBG_STACKED_H;
	unsigned count = vm->ls+1;
	unsigned sc = count > ny ? count - ny: 0;
	unsigned x = DBG_STACK_W+2+DBG_CSTACK_W+1+DBG_NODE_W+1;
	draw_cls_rect(x, DBG_HEADER_H+2, DBG_NODE_W, DBG_STACKED_H);
	while( ny-->0 && sc < count){
		term_gotoxy(x, y--);
		printf(" $(%3u) %ld", sc, vm->save[sc] ? vm->save[sc] - txt : -1);
		++sc;
	}
}

__private void draw_range_clr(void){
	draw_cls_rect(1, DBG_HEADER_H+2+DBG_STACKED_H+1, DBG_SCREEN_W, DBG_RANGE_H);
}

__private void draw_range(lips_s* vm, unsigned idrange, long ch){
	const unsigned cunset = 130;
	const unsigned cerr   = 124;
	const unsigned cset   = 22;
	const unsigned cmatch = 17;
	draw_range_clr();
	term_gotoxy(1, DBG_HEADER_H+2+DBG_STACKED_H+1);
	printf("range:%4u ch:", idrange);
	if( ch == -1 ){
		term_bcol(cunset);
	}
	else{
		term_bcol( range(vm, idrange, ch)? cmatch: cerr);
	}
	putc_view_char(ch);
	term_reset();
	printf("  ");
	for( unsigned i = '\t'; i < 128; ++i ){
		if( i != '\t' && i != '\n' && i < 32 ) continue;
		if( range(vm, idrange, i ) ){
			if( ch == i ) term_bcol(cmatch);
			else term_bcol(cset);
		}
		else{
			if( ch == i ) term_bcol(cerr);
			else term_bcol(cunset);
		}
		putc_view_char(i);
	}
	term_reset();
}

__private void draw_input(lips_s* vm, const utf8_t* txt, unsigned len, int oncol){
	unsigned y = DBG_HEADER_H+2+DBG_STACKED_H+1+DBG_RANGE_H+1+DBG_CODE_H+1;
	draw_cls_rect(1, y, DBG_SCREEN_W, DBG_INP_H);
	unsigned const ls = m_header(vm->stack)->len-1;
	unsigned offx = vm->stack[ls].sp - txt;
	long low = offx;
	low -= DBG_SCREEN_W/2;
	if( low < 0 ) low = 0;

	unsigned high = low + DBG_SCREEN_W;

	if( high > len ){
		high = len;
		low = (long)high - DBG_SCREEN_W;
		if( low < 0 ) low = 0;
	}
	term_gotoxy(1, y);
	for( unsigned i = low; i < high; ++i ){
		if( &txt[i] == vm->stack[ls].sp ){
			switch( oncol ){
				case 0: term_fcol(160); break;
				case 1: term_fcol(46); break;
			}
			putc_view_char(txt[i]);
			term_reset();
			offx = i-low;
		}
		else{
			putc_view_char(txt[i]);
		}
	}
	term_gotoxy(1+offx, y+1);
	switch( oncol ){
		case 0: term_fcol(160); break;
		case 1: term_fcol(46); break;
	}
	putchar('^');
	term_reset();
}

__private void draw_opcode(lips_s* vm, unsigned pc, const char** nmap, unsigned curpc, const utf8_t* txt, unsigned len){
	unsigned byc = vm->code[pc];
	switch( BYTECODE_CMD40(byc) ){
		default : printf("INTERNAL ERROR CMD40: 0x%X", BYTECODE_CMD40(byc)); break;
		case OP_RANGE :
			printf("range %u", BYTECODE_VAL12(byc));
			if( curpc == pc ){
				term_reset();
				draw_range(vm, BYTECODE_VAL12(byc), *vm->stack[m_header(vm->stack)->len-1].sp);
				draw_input(vm, txt, len, !!range(vm, BYTECODE_VAL12(byc), *vm->stack[m_header(vm->stack)->len-1].sp));
			}
		break;
		case OP_URANGE: break;
		case OP_MATCH :  fputs("match", stdout); break;
		case OP_CHAR  :
			fputs("char  ", stdout); putc_view_char(BYTECODE_VAL08(byc)); 
			if( curpc == pc ){
				term_reset();
				draw_input(vm, txt, len, BYTECODE_VAL08(byc) == *vm->stack[m_header(vm->stack)->len-1].sp);
			}
		break;
		case OP_SPLITF: printf("split %X", pc + BYTECODE_VAL12(byc)); break;
		case OP_SPLITB: printf("split %X", pc - BYTECODE_VAL12(byc)); break;
		case OP_SPLIRF: printf("splir %X", pc + BYTECODE_VAL12(byc)); break;
		case OP_SPLIRB: printf("splir %X", pc - BYTECODE_VAL12(byc)); break;
		case OP_JMPF  : printf("jmp   %X", pc + BYTECODE_VAL12(byc)); break;
		case OP_JMPB  : printf("jmp   %X", pc - BYTECODE_VAL12(byc)); break;
		case OP_NODE  : printf("node  new::%s::%u", nmap[BYTECODE_VAL12(byc)],BYTECODE_VAL12(byc)); break;
		case OP_CALL  : printf("call  [%u]%s -> %X",BYTECODE_VAL12(byc), nmap[BYTECODE_VAL12(byc)], vm->fn[BYTECODE_VAL12(byc)]); break;
		case OP_EXT:
			switch(BYTECODE_CMD04(byc)){
				case OPE_SAVE  : printf("save  %u", BYTECODE_VAL08(byc)); break;
				case OPE_RET   : printf("ret   %u", BYTECODE_VAL08(byc)); break;
				case OPE_NODEEX:
					switch( BYTECODE_VAL08(byc) ){
						case NOP_PARENT : printf("node   parent"); break;
						case NOP_DISABLE: printf("node   disable"); break;
						case NOP_ENABLE : printf("node   enable"); break;
					}
				break;
				case OPE_ERROR: printf("error %u", BYTECODE_VAL08(byc)); break;
				default: printf("INTERNAL ERROR CMD04: 0x%X", BYTECODE_CMD04(byc)); break;
			}
		break;
	}
}

__private void draw_code(lips_s* vm, unsigned pc, const char** nmap, const utf8_t* txt, unsigned len){
	unsigned y = DBG_HEADER_H+2+DBG_STACKED_H+1+DBG_RANGE_H+1;
	unsigned const count = vm->bytecode[BYC_CODELEN];
	unsigned const csel  = 63;
	draw_cls_rect(1, y, DBG_SCREEN_W, DBG_CODE_H);
	long low  = pc;
   	low -= DBG_CODE_H/2;
	if( low < 0 ) low = 0;
	unsigned high = low + DBG_CODE_H;
	if( high > count ){
		high = count;
		low = (long)high - DBG_CODE_H;
		if( low < 0 ) low = 0;
	}
	for(unsigned i = low; i < high; ++i ){
		term_gotoxy(1, y++);
		if( pc == i ){
			term_fcol(csel);
			printf(">");
		}
		else{
			printf(" ");
		}
		printf("%04X  ", i);
		draw_opcode(vm, i, nmap, pc, txt, len);
		term_reset();
	}
}

__private char* prompt(void){
	unsigned y =  DBG_HEADER_H+2+DBG_STACKED_H+1+DBG_RANGE_H+1+DBG_CODE_H+1+DBG_INP_H+1;
	draw_cls_rect(1, y, DBG_SCREEN_W, DBG_PROMPT_H);
	term_gotoxy(1, y);
	return readline("> ");
}

__private void onend(void){
	term_gotoxy(0, DBG_HEADER_H+2+DBG_STACKED_H+1+DBG_RANGE_H+1+DBG_CODE_H+1+DBG_PROMPT_H+1+DBG_INP_H+1);
	fflush(stdout);
}

__private void redraw(lipsdbg_s* l){
	draw_range_clr();
	draw_header(l->vm.sectionStart, l->vm.bytecode[BYC_RANGE_COUNT], l->vm.bytecode[BYC_FN_COUNT], l->curstk);
	draw_stack(l->vm.stack, l->source);
	draw_cstack(&l->vm);
	draw_node(&l->vm, l->nmap, l->source);
	draw_save(&l->vm, l->source);
	if( l->curstk ){
		draw_input(&l->vm, l->source, l->srclen, -1);
		draw_code(&l->vm, l->vm.stack[l->curstk-1].pc, l->nmap, l->source, l->srclen);
	}
	fflush(stdout);
}

__private void ast_dump_file(lipsAst_s* ast, const char** nmap, unsigned tab, FILE* f){
	for( unsigned i = 0; i < tab; ++i ){
		fputs("  ", f);
	}
	if( m_header(ast->child)->len ){
		fprintf(f, "<%s>\n", ast->id == LIPS_NODE_START ? "_start_": nmap[ast->id]);
		mforeach(ast->child,i){
			ast_dump_file(&ast->child[i], nmap, tab+1, f);
		}
	}
	else{
		fprintf(f, "<%s> '", ast->id == LIPS_NODE_START ? "_start_": nmap[ast->id]);
		for( unsigned i = 0; i < ast->len; ++i ){
			fputs(cast_view_char(ast->sp[i], 0), f);
		}
		fputs("'\n", f);
	}
}

__private void dump_dot(lipsAst_s* ast, const char** nmap, FILE* f, const char* parent, unsigned index){
	__free char* name = str_printf("%s_%s_%d", parent, ast->id == LIPS_NODE_START ? "_start_": nmap[ast->id], index);
	if( *parent ) fprintf(f, "%s -> %s;\n",parent, name);

	if( m_header(ast->child)->len ){
		fprintf(f, "%s [label=\"%s\"];\n", name, ast->id == LIPS_NODE_START ? "_start_": nmap[ast->id]);
		mforeach(ast->child,i){
			dump_dot(&ast->child[i], nmap, f, name, i);
		}
	}
	else{
		fprintf(f, "%s [label=\"%s\\n", name, ast->id == LIPS_NODE_START ? "_start_": nmap[ast->id]);
		for( unsigned i = 0; i < ast->len; ++i ){
			fputs(cast_view_char(ast->sp[i], 0), f);
		}
		fprintf(f, "\"];\n");
	}
}

__private void ast_dump_dot(lipsAst_s* ast, const char** nmap, FILE* f){
	fputs("digraph {\n", f);
	dump_dot(ast, nmap, f, "", 0);
	fputs("}\n", f);
	fclose(f);
}


__private long find_bp(uint32_t* bp, uint32_t val){
	mforeach(bp, i)
		if( bp[i] == val ) return i;
	return -1;
}

__private void action_reuse(__unused lipsdbg_s* l, __unused char** args){}

__private void action_step(lipsdbg_s* l, __unused char** args){
	l->run = DCMD_STEP;
}

__private void action_next(lipsdbg_s* l, __unused char** args){
	if( !l->curstk ){
		l->next[0] = l->next[1] = -1;
		return;
	}
	l->next[0] = l->vm.stack[l->curstk-1].pc + 1;
	l->next[1] = l->curstk > 1 ? l->vm.stack[l->curstk-2].pc: -1;
	l->run = DCMD_CONTINUE;
}

__private void action_continue(lipsdbg_s* l, __unused char** args){
	l->run = DCMD_CONTINUE;
}

__private void action_quit(lipsdbg_s* l, __unused char** args){
	l->run = DCMD_QUIT;
}

__private void action_breakpoint(lipsdbg_s* l, char** args){
	unsigned ibp = strtoul(args[1], NULL, 16);
	if( ibp < l->vm.bytecodeLen && find_bp(l->breakpoint, ibp) < 0 ){
		unsigned j = m_ipush(&l->breakpoint);
		l->breakpoint[j] = ibp;
	}
}

__private void action_breakremove(lipsdbg_s* l, char** args){
	unsigned ibp = strtoul(args[1], NULL, 16);
	long ir = find_bp(l->breakpoint, ibp);
	if( ir >= 0 ) m_delete(l->breakpoint, ir, 1);
}

typedef struct cmdmap{
	const char* shr;
	const char* lng;
	dbgaction_f act;
	unsigned    nam;
}cmdmap_s;

cmdmap_s CMD[] = {{
		.shr = "",
		.lng = "",
		.act = action_reuse,
		.nam = 0
	},{
		.shr = "s",
		.lng = "step",
		.act = action_step,
		.nam = 0
	},{
		.shr = "n",
		.lng = "next",
		.act = action_next,
		.nam = 0
	},{
		.shr = "c",
		.lng = "continue",
		.act = action_continue,
		.nam = 0
	},{
		.shr = "q",
		.lng = "quit",
		.act = action_quit,
		.nam = 0
	},{
		.shr = "b",
		.lng = "breakpoint",
		.act = action_breakpoint,
		.nam = 1
	},{
	},{
		.shr = "br",
		.lng = "breakremove",
		.act = action_breakremove,
		.nam = 1
	}
};

long debug_command_arg_address(const char* from, unsigned byclen){
	unsigned ret = strtoul(from, NULL, 16);
	return ret < byclen ? ret: -1;
}

__private void debug_command(lipsdbg_s* l){
	if( l->curstk && find_bp(l->breakpoint, l->vm.stack[l->curstk-1].pc) >= 0 ){
		l->run = DCMD_SUSPEND;
	}
	else if( l->run == DCMD_CONTINUE ){
		if( l->curstk ){
			if( l->next[0] > -1 && l->next[0] == l->vm.stack[l->curstk-1].pc ) l->run = DCMD_SUSPEND;
			else if( l->next[1] > -1 && l->next[1] == l->vm.stack[l->curstk-1].pc ) l->run = DCMD_SUSPEND;
			else return;
		}
		else{
			return;
		}
	}
	
	do{
		char* inp = prompt();
		char** args = str_splitin(inp, " ", 0);
		if( m_header(args)->len == 0 ){
			unsigned i = m_ipush(&args);
			args[i] = inp;
		}
		iassert(m_header(args)->len);
		for( unsigned i = 0; i < sizeof_vector(CMD); ++i ){
			if( !strcmp(args[0], CMD[i].shr) || !strcmp(args[0], CMD[i].lng) ){
				if( CMD[i].nam == m_header(args)->len-1 ) CMD[i].act(l, args);
				CMD[0].act = CMD[i].act;
				break;
			}
		}
		free(inp);
		m_free(args);
	}while( l->run == DCMD_SUSPEND );
}

int lips_debug(uint16_t* bytecode, const utf8_t* txt, lipsMatch_s* ret){
	ret->match = 0;
	ret->ast   = NULL;
	ret->err   = NULL;
	
	lipsdbg_s l;
	lips_ctor(&l.vm, bytecode, txt);
	stk_push(&l.vm, l.vm.sectionStart, l.vm.sp);
	l.nmap = lips_map_name(bytecode);
	l.breakpoint = MANY(uint32_t, 4);
	l.source = txt;
	l.srclen = strlen((char*)txt);
	l.run = DCMD_SUSPEND;

	draw_clear();
	do{
		l.curstk = m_header(l.vm.stack)->len;
		redraw(&l);
		debug_command(&l);
		if( l.run == DCMD_QUIT ) break;
	}while( stk_pop(&l.vm) && vm_exec(&l.vm, ret) );
	term_cls();
	if( ret->match < 1 ) ret->err = m_borrowed(l.vm.errors);
	lips_dtor(&l.vm);
	m_free(l.nmap);
	m_free(l.breakpoint);
	return ret->match;
}

void lips_dump_capture(lipsMatch_s* m, FILE* f){
	if( m->match == 0 ){
		fputs("lips capture: no match\n", f);
	}
	else if( m->match < 0 ){
		fputs("lips capture: error, this never append for now\n", f);
	}
	else{
		for( int i = 0; i < m->match; ++i ){
			unsigned st = i*2;
			unsigned en = st+1;
			unsigned len = m->capture[en] - m->capture[st];
			fprintf(f, "<capture%u>%.*s</capture%u>\n", i, len, m->capture[st], i);
		}
	}
}

void lips_dump_ast(lipsMatch_s* m, uint16_t* bytecode, FILE* f, int term, int dot){
	__free const char** nmap = lips_map_name(bytecode);
	if( !m->ast ) return;
	if( m->match < 1 ) return;
	if( term ) ast_dump_file(m->ast, nmap, 0, f);
	if( dot  ) ast_dump_dot(m->ast, nmap, f);
}

__private int sort_err(const void* a, const void* b){
	const lipsError_s* ea = a;
	const lipsError_s* eb = b;
	if( ea->loc < eb->loc ) return -1;
	if( ea->loc > eb->loc ) return 1;
	return 0;
}

void lips_dump_error(lipsMatch_s* m, const utf8_t* source, FILE* f){
	if( m->match < 0 ){
		fprintf(f, "lips error: internal error, this never append for now\n");
	}
	else if( m->match > 0 ){
		fprintf(f, "lips error: no errors, lips have match\n");
	}
	else{
		unsigned const count = m_header(m->err)->len;
		if( count ){
			if( !(m->err[count-1].number & LIPS_ERROR_DIE) ){
				m_qsort(m->err, sort_err);
			}
			unsigned num  = m->err[count-1].number;
			unsigned fail = num & LIPS_ERROR_DIE;
			num &= ~LIPS_ERROR_DIE;
			fprintf(f, "lips %s: %u\n", fail?"fail":"error", num);
			const char*    sp    = (const char*)m->err[count-1].loc;
			const unsigned len   = strlen((const char*)source);
			const unsigned iline = count_line_to((const char*)source, sp);
			unsigned       offv  = fprintf(f, "%04u | ", iline);
			const char*    eline = NULL;
			const char*    sline = extract_line((const char*)source, (const char*)source+len, sp, &eline);
			for( const char* p = sline; p < eline; ++p ){
				fputs(cast_view_char(*p, 0), f);
				if( p < sp ) ++offv;
			}
			fputc('\n', f);
			for( unsigned i = 0; i < offv; ++i ){
				fputc(' ', f);
			}
			fputs("^\n", f);
		}
		else{
			fprintf(f, "lips error: match not have mark any type of error but not have match\n");
		}
	}
}


























