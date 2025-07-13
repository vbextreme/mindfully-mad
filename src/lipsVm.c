#include <lips/vm.h>

__private void stk_push(lipsVM_s* vm, unsigned pc, const utf8_t* sp){
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
	m_header(vm->stack)->len = pos-1;
}

lipsVM_s* lips_vm_ctor(lipsVM_s* vm, lipsByc_s* byc){
	vm->byc      = byc;
	vm->stack    = MANY(lipsStack_s, vm->byc->codeLen);
	vm->cstk     = MANY(unsigned, 128);
	vm->node     = MANY(lipsAsl_s, vm->byc->fnCount);
	return vm;
}

void lips_vm_dtor(lipsVM_s* vm){
	m_free(vm->stack);
	m_free(vm->cstk);
	m_free(vm->node);
}

void lips_vm_reset(lipsVM_s* vm, lipsMatch_s* match, const utf8_t* txt){
	m_clear(vm->stack);
	m_clear(vm->cstk);
	m_clear(vm->node);
	if( txt ) vm->txt = txt;
	vm->ls = -1;
	vm->match = match;
	memset(vm->match->capture, 0, sizeof vm->match->capture);
	vm->match->err.number = 0;
	vm->match->err.loc    = NULL;
	vm->match->ast        = NULL;
	vm->match->count      = -1;
	stk_push(vm, vm->byc->start, vm->sp);
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
	while( stk_pop(vm) && lips_vm_exec(vm) );
	return vm->match->count;
}























