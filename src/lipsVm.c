#include <lips/vm.h>

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
*/

__private void stk_push(lipsVM_s* vm, unsigned pc, const utf8_t* sp){
	if( pc >= vm->byc->codeLen ) return;
	unsigned i = m_ipush(&vm->stack);
	vm->stack[i].pc = pc;
	vm->stack[i].sp = sp;
	vm->stack[i].ls = vm->ls;
	vm->stack[i].cp = m_header(vm->cstk)->len;
	vm->stack[i].np = m_header(vm->node)->len;
}

__private int stk_pop(lipsVM_s* vm){
	long i = m_ipop(vm->stack);
	if( i < 0 ) return 0;
	vm->pc = vm->stack[i].pc;
	vm->sp = vm->stack[i].sp;
	vm->ls = vm->stack[i].ls;
	m_header(vm->cstk)->len = vm->stack[i].cp;
	m_header(vm->node)->len = vm->stack[i].np;
	return 1;
}

__private void stk_return_call_success(lipsVM_s* vm, unsigned pos){
	iassert(pos>0);
	vm->stack[pos-1].sp = vm->sp;
	vm->stack[pos-1].ls = vm->ls;
	vm->stack[pos-1].np = m_header(vm->node)->len;
	m_header(vm->stack)->len = pos;
}

__private void stk_return_call_fail(lipsVM_s* vm, unsigned pos){
	iassert(pos>0);
	if( pos ){
		m_header(vm->stack)->len = pos-1;
	}
	else{
		m_header(vm->stack)->len = 0;
	}
}

lipsVM_s* lips_vm_ctor(lipsVM_s* vm, lipsByc_s* byc){
	dbg_info("");
	vm->byc      = byc;
	vm->stack    = MANY(lipsStack_s, vm->byc->codeLen);
	vm->cstk     = MANY(unsigned, 128);
	vm->node     = MANY(lipsAsl_s, vm->byc->fnCount);
	return vm;
}

void lips_vm_dtor(lipsVM_s* vm){
	dbg_info("");
	m_free(vm->stack);
	m_free(vm->cstk);
	m_free(vm->node);
}

lipsMatch_s* lips_match_ctor(lipsMatch_s* match){
	match->err.asl = MANY(lipsAsl_s,1024);
	match->ast     = NULL;
	return match;
}

void lips_match_dtor(lipsMatch_s* match){
	lips_ast_dtor(match->ast);
	m_free(match->err.asl);
}

__private void lips_match_reset(lipsMatch_s* match){
	memset(match->capture, 0, sizeof match->capture);
	match->err.number = 0;
	match->err.loc    = NULL;
	match->ast        = NULL;
	match->count      = 0;
	m_clear(match->err.asl);
}

void lips_vm_reset(lipsVM_s* vm, lipsMatch_s* match, const utf8_t* txt){
	dbg_info("");
	m_clear(vm->stack);
	m_clear(vm->cstk);
	m_clear(vm->node);
	if( txt ) vm->txt = txt;
	vm->ls = -1;
	vm->match = match;
	lips_match_reset(vm->match);
	stk_push(vm, vm->byc->start, txt);
}

__private int op_range(lipsVM_s* vm, unsigned ir, utf8_t ch){
	const unsigned im = ch & 0x0F;
	const unsigned bm = 1 << (ch>>4);
	return vm->byc->range[ir*16+im] & bm;
}

__private void op_save(lipsVM_s* vm, unsigned id){
	iassert(id < 256);
	vm->match->capture[id] = vm->sp;
	if( (int)id > vm->ls ) vm->ls = id;
}

__private void op_call(lipsVM_s* vm, unsigned ifn){
	stk_push(vm, vm->pc+1, vm->sp);
	unsigned i = m_ipush(&vm->cstk);
	vm->cstk[i] = m_header(vm->stack)->len;
	stk_push(vm, vm->byc->fn[ifn], vm->sp);
}

__private void op_ret(lipsVM_s* vm, int fail){
	long i = m_ipop(vm->cstk);
	if( i >=0 ){
		if( fail ){
			if( vm->sp > vm->match->err.loc ){
				vm->match->err.loc = vm->sp;
				vm->match->err.asl = m_copy(vm->match->err.asl, vm->node, 0);
			}
			stk_return_call_fail(vm, vm->cstk[i]);
		}
		else{
			stk_return_call_success(vm, vm->cstk[i]);
		}
	}
}

__private void op_error(lipsVM_s* vm, uint8_t num){
	iassert(num < m_header(vm->byc->errStr)->len);
	vm->match->err.loc    = vm->sp;
	vm->match->err.number = num;
	m_clear(vm->stack);
}

__private void op_node(lipsVM_s* vm, nodeOP_e op, unsigned id){
	unsigned i = m_ipush(&vm->node);
	vm->node[i].sp = vm->sp;
	vm->node[i].id = id;
	vm->node[i].op = op;
}

int lips_vm_exec(lipsVM_s* vm){
	if( !stk_pop(vm) ) return 0;
	const unsigned byc = vm->byc->code[vm->pc];
	switch( BYTECODE_CMD40(byc) ){
		case OP_MATCH:
			op_save(vm, 1);
			vm->match->count = (vm->ls+1)/2;
			if( m_header(vm->node)->len ) vm->match->ast = lips_ast_make(vm->node);
		return 0;
		
		case OP_CHAR:
			if( *vm->sp == BYTECODE_VAL08(byc) ){
				stk_push(vm, vm->pc+1, vm->sp+1);
			}
		break;
		
		case OP_RANGE:
			if( op_range(vm, BYTECODE_VAL12(byc), *vm->sp) ){
				stk_push(vm, vm->pc+1, vm->sp+1);
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
			op_node(vm, NOP_NEW, BYTECODE_VAL12(byc));
			stk_push(vm, vm->pc+1, vm->sp);	
		break;
		
		case OP_CALL:
			op_call(vm, BYTECODE_VAL12(byc));
		break;
		
		case OP_EXT:
			switch(BYTECODE_CMD04(byc)){
				case OPE_SAVE:
					op_save(vm, BYTECODE_VAL08(byc));
					stk_push(vm, vm->pc+1, vm->sp);
				break;
				
				case OPE_RET:
					op_ret(vm, BYTECODE_VAL08(byc));
				break;
				
				case OPE_NODEEX:
					op_node(vm, BYTECODE_VAL08(byc), 0);
					stk_push(vm, vm->pc+1, vm->sp);
				break;
				
				case OPE_ERROR:
					op_error(vm, BYTECODE_VAL08(byc));
				break;
			}
		break;
	}
	return -1;
}

int lips_vm_match(lipsVM_s* vm){
	while( lips_vm_exec(vm) );
	return vm->match->count;
}

int lips_range_test(lipsVM_s* vm, unsigned ir, utf8_t ch ){
	return op_range(vm, ir, ch);
}





















