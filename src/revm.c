#include <notstd/utf8.h>
#include <notstd/memory.h>
#include <mfmrevm.h>
#include <readline/readline.h>

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



typedef struct revmStack{
	unsigned      pc;
	const utf8_t* sp;
	unsigned      ls;
	unsigned      cp;
	unsigned      np;
}revmStack_s;

typedef struct revm{
	uint16_t*      bytecode;
	uint16_t*      fn;
	uint16_t*      range;
	revmStack_s*   stack;
	const utf8_t*  save[REVM_MAX_CAPTURE*2];
	unsigned*      cstk;
	bcnode_s*      node;
	const utf8_t*  sp;
	uint32_t       start;
	uint32_t       pc;
	unsigned       ls;
}revm_s;

__private void stk_push(revm_s* vm, unsigned pc, const utf8_t* sp){
	unsigned i = m_ipush(&vm->stack);
	vm->stack[i].pc = pc;
	vm->stack[i].sp = sp;
	vm->stack[i].ls = vm->ls;
	vm->stack[i].cp = m_header(vm->cstk)->len;
	vm->stack[i].np = m_header(vm->node)->len;
}

__private int stk_pop(revm_s* vm){
	long i = m_ipop(vm->stack);
	if( i < 0 ) return 0;
	vm->pc = vm->stack[i].pc;
	vm->sp = vm->stack[i].sp;
	vm->ls = vm->stack[i].ls;
	m_header(vm->cstk)->len = vm->stack[i].cp;
	m_header(vm->cstk)->len = vm->stack[i].np;
	return 1;
}

__private int range(revm_s* vm, unsigned ir, utf8_t ch){
	const unsigned im = ch & 0x0F;
	const unsigned bm = 1 << (ch>>4);
	return vm->range[ir+im] & bm;
}

__private void save(revm_s* vm, unsigned id){
	iassert(id < 256);
	vm->save[id] = vm->sp;
	if( id > vm->ls ) vm->ls = id;
}

__private void call(revm_s* vm, unsigned ifn){
	stk_push(vm, vm->start + vm->fn[ifn], vm->sp);
	unsigned i = m_ipush(&vm->cstk);
	vm->cstk[i] = vm->pc+1;
}

__private void cret(revm_s* vm){
	long i = m_ipop(vm->cstk);
	if( i>= 0 ) stk_push(vm, vm->cstk[i], vm->sp);
}

__private void revm_dtor(revm_s* vm){
	m_free(vm->stack);
	m_free(vm->cstk);
	m_free(vm->node);
}

__private void node(revm_s* vm, nodeOP_e op, unsigned id){
	unsigned i = m_ipush(&vm->node);
	vm->node[i].sp = vm->sp;
	vm->node[i].id = id;
	vm->node[i].op = op;
}

__private int vm_exec(revm_s* vm, revmMatch_s* ret){
	const unsigned byc = vm->bytecode[vm->pc];
	switch( BYTECODE_CMD40(byc) ){
		case OP_MATCH:
			save(vm, 1);
			memcpy(ret->capture, vm->save, sizeof(const utf8_t*) * (vm->ls+1));
			ret->match = (vm->ls+1)/2;
			ret->node  = m_borrowed(vm->node);
			revm_dtor(vm);
		return 0;
		
		case OP_CHAR:
			if( *vm->sp == BYTECODE_VAL08(byc) ){
				stk_push(vm, vm->pc+1, vm->sp+1);
			}
			else{
				cret(vm);
			}
		break;
		
		case OP_RANGE:
			if( range(vm, BYTECODE_VAL12(byc), *vm->sp) ){
				stk_push(vm, vm->pc+1, vm->sp+1);
			}
			else{
				cret(vm);
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
		break;
		
		case OP_CALL:
			call(vm, BYTECODE_CMD40(byc));
		break;
		
		case OP_EXT:
			switch(BYTECODE_CMD04(byc)){
				case OPE_SAVE:
					save(vm, BYTECODE_VAL08(byc));
				break;
				
				case OPE_RET:
					cret(vm);
				break;
				
				case OPE_PARENT:
					node(vm, NOP_PARENT, 0);
				break;
			}
		break;
	}
	return -1;
}

revmMatch_s revm_match(uint16_t* bytecode, const utf8_t* txt){
	revmMatch_s ret = {.match = 0, .node = NULL};
	
	revm_s vm;
	vm.bytecode = bytecode;
	vm.fn       = &bytecode[bytecode[BYC_SECTION_FN]];
	vm.range    = &bytecode[bytecode[BYC_SECTION_RANGE]];
	vm.pc       = 0;
	vm.stack    = MANY(revmStack_s, bytecode[BYC_CODELEN]);
	vm.cstk     = MANY(unsigned, 128);
	vm.node     = MANY(bcnode_s, 32);
	vm.start    = bytecode[BYC_START];
	vm.sp       = txt;
	
	stk_push(&vm, vm.start, vm.sp);
	while( stk_pop(&vm) && vm_exec(&vm, &ret) );
	revm_dtor(&vm);
	return ret;
}

__private void term_cls(void){
	fputs("\033[2J\033[H", stdout);
}

__private void term_gotoxy(unsigned x, unsigned y){
	printf("\033[%d;%dH", x+1, y+1);
}

__private void term_fcol(unsigned col){
	printf("\033[38;5;%umm", col);
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


/*
	start:

	stack     |    node |  fname  | range
	1         | 1       | 1       | 1
	2         | 2       | 2       | 2
	3         | 3       | 3       | 3
    -------------------------------------------
	command
	1
	2
	3
	4
	5
	
	>

*/
void revm_debug(uint16_t* bytecode, const utf8_t* txt){
	
	
	
	
	
	return;
	revmMatch_s ret = {.match = 0, .node = NULL};
	
	revm_s vm;
	vm.bytecode = bytecode;
	vm.fn       = &bytecode[bytecode[BYC_SECTION_FN]];
	vm.range    = &bytecode[bytecode[BYC_SECTION_RANGE]];
	vm.pc       = 0;
	vm.stack    = MANY(revmStack_s, bytecode[BYC_CODELEN]);
	vm.cstk     = MANY(unsigned, 128);
	vm.node     = MANY(bcnode_s, 32);
	vm.start    = bytecode[BYC_START];
	vm.sp       = txt;
	
	stk_push(&vm, vm.start, vm.sp);
	while( stk_pop(&vm) && vm_exec(&vm, &ret) );
	revm_dtor(&vm);
}


































