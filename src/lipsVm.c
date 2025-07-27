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

__private void stk_push(lipsVM_s* vm, unsigned pc){
	if( pc >= vm->byc->codeLen ) return;
	unsigned i = m_ipush(&vm->stack);
	vm->stack[i].pc = pc;
	vm->stack[i].sp = vm->sp;
	vm->stack[i].ls = vm->match->count;
	vm->stack[i].cp = m_header(vm->cstk)->len;
	vm->stack[i].np = m_header(vm->node)->len;
	vm->stack[i].ip = vm->ip;
}

__private int stk_pop(lipsVM_s* vm){
	long i = m_ipop(vm->stack);
	if( i < 0 ) return 0;
	vm->pc = vm->stack[i].pc;
	vm->sp = vm->stack[i].sp;
	vm->ip = vm->stack[i].ip;
	vm->match->count = vm->stack[i].ls;
	m_header(vm->cstk)->len = vm->stack[i].cp;
	m_header(vm->node)->len = vm->stack[i].np;
	return 1;
}

__private int stk_pop_pc(lipsVM_s* vm){
	long i = m_ipop(vm->stack);
	if( i < 0 ) return 0;
	vm->pc = vm->stack[i].pc;
	return 1;
}

__private lipsScope_s* scope_new(lipsScope_s* parent){
	lipsScope_s* scope = NEW(lipsScope_s);
	scope->parent  = parent;
	scope->child   = NULL;
	scope->symbols = MANY(lipsAst_s*, 32);
	return scope;
}

lipsVM_s* lips_vm_ctor(lipsVM_s* vm, lipsByc_s* byc){
	dbg_info("");
	vm->byc      = byc;
	vm->stack    = MANY(lipsStack_s, vm->byc->codeLen);
	vm->cstk     = MANY(unsigned, 128);
	vm->node     = MANY(lipsAsl_s, vm->byc->fnCount);
	vm->scope    = scope_new(NULL);
	return vm;
}

__private void scope_dtor(lipsScope_s* scope){
	if( !scope ) return;
	scope_dtor(scope->child);
	m_free(scope->symbols);
	m_free(scope);
}

void lips_vm_dtor(lipsVM_s* vm){
	dbg_info("");
	m_free(vm->stack);
	m_free(vm->cstk);
	m_free(vm->node);
	scope_dtor(vm->scope);
}

lipsMatch_s* lips_match_ctor(lipsMatch_s* match){
	match->ast.root   = NULL;
	match->ast.marked = NULL;
	return match;
}

void lips_match_dtor(lipsMatch_s* match){
	lips_ast_match_dtor(&match->ast);
}

__private void lips_match_reset(lipsMatch_s* match){
	memset(match->capture, 0, sizeof match->capture);
	lips_ast_match_dtor(&match->ast);
	match->err.number = 0;
	match->err.loc    = NULL;
	match->err.last   = NULL;
	match->ast.root   = NULL;
	match->ast.marked = NULL;
	match->count      = 0;
}

void lips_vm_reset(lipsVM_s* vm, lipsMatch_s* match, const utf8_t* txt){
	//dbg_info("");
	m_clear(vm->stack);
	m_clear(vm->cstk);
	m_clear(vm->node);
	vm->ip = NULL;
	if( txt ) vm->txt = txt;
	vm->match = match;
	lips_match_reset(vm->match);
	vm->er = 0;
	vm->pc = vm->byc->start;
	vm->sp = txt;
}

void lips_vm_semantic_reset(lipsVM_s* vm){
	m_clear(vm->stack);
	m_clear(vm->cstk);
	m_clear(vm->node);
	vm->er = 0;
	vm->ip = 0;
}

__private int op_range(lipsVM_s* vm, unsigned ir, utf8_t ch){
	const unsigned im = ch & 0x0F;
	const unsigned bm = 1 << (ch>>4);
	return vm->byc->range[ir*16+im] & bm;
}

__private void op_save(lipsVM_s* vm, unsigned id){
	iassert(id < 256);
	vm->match->capture[id] = vm->sp;
	if( (int)id > vm->match->count ) vm->match->count = id;
}

__private void op_call(lipsVM_s* vm, unsigned ifn){
	stk_push(vm, vm->pc+1);
	unsigned i = m_ipush(&vm->cstk);
	vm->cstk[i] = m_header(vm->stack)->len;
	vm->pc = vm->byc->fn[ifn];
}

__private int op_ret(lipsVM_s* vm, int fail){
	long i = m_ipop(vm->cstk);
	if( i >= 0 ){
		if( fail ){
			iassert( vm->cstk[i] );
			m_header(vm->stack)->len = vm->cstk[i]-1;
			return stk_pop(vm);
		}
		else{
			m_header(vm->stack)->len = vm->cstk[i];
			return stk_pop_pc(vm);
		}
	}
	return 0;
}

__private void op_error(lipsVM_s* vm, uint8_t num){
	iassert(num < m_header(vm->byc->errStr)->len);
	if( !num ){
		vm->match->err.loc = NULL;
		vm->match->err.number = 0;
		vm->er = 0;
	}
	else{
		//dbg_info("set err %u", num);
		vm->er = num;
	}
}

__private void on_error(lipsVM_s* vm){
	if( vm->sp >= vm->match->err.loc && vm->er ){
		//dbg_info("store error pos %lu num %u", vm->sp-vm->txt, vm->rerr);
		vm->match->err.number = vm->er;
		vm->match->err.loc    = vm->sp;
	}
}

__private void op_node(lipsVM_s* vm, lipsOP_e op, unsigned id){
	unsigned i = m_ipush(&vm->node);
	vm->node[i].sp = vm->sp;
	vm->node[i].id = id;
	vm->node[i].op = op;
}

__private int op_value(lipsVM_s* vm, unsigned mode){
	unsigned len;
	const char* str = lipsByc_extract_string(vm->byc->bytecode, vm->pc, &vm->pc, &len);
	switch( mode ){
		case OPEV_VALUE_SET:
			vm->ip->orgtp  = vm->ip->tp;
			vm->ip->orglen = vm->ip->len;
			vm->ip->len    = len;
			vm->ip->tp     = (const utf8_t*)str;
		return 1;
		
		case OPEV_VALUE_TEST:
			if( vm->ip->len != len ) return 0;
			if( strncmp((char*)vm->ip->tp, str, len) ) return 0;
		return 1;
	}
	die("internal error, opev_value unknown %u", mode);
	return 0;
}

__private lipsAst_s* scope_symbol_search_content(lipsScope_s* scope, unsigned id, lipsAst_s* node){
	if( !scope ) return NULL;
	mforeach(scope->symbols, i){
		if( scope->symbols[i]->id == id && scope->symbols[i]->len == node->len && !strncmp((const char*)scope->symbols[i]->tp, (const char*)node->tp, node->len)) 
			return scope->symbols[i];
	}
	return scope_symbol_search_content(scope->parent, id, node);
}

__private void scope_symbol_add(lipsScope_s* scope, lipsAst_s* node){
	unsigned i = m_ipush(&scope->symbols);
	scope->symbols[i] = node;
}

__private void op_scope(lipsVM_s* vm, unsigned mode){
	switch(mode){
		case OPEV_SCOPE_NEW:
			vm->sc->child = scope_new(vm->sc);
			vm->sc = vm->sc->child;
		break;
		
		case OPEV_SCOPE_LEAVE:
			if( !vm->sc->parent ) break;
			vm->sc = vm->sc->parent;
			scope_dtor(vm->sc->child);
			vm->sc->child = NULL;
		break;
		
		case OPEV_SCOPE_SYMBOL:
			scope_symbol_add(vm->sc, vm->ip);
		break;
	}
}

int lips_vm_exec(lipsVM_s* vm){
	const unsigned byc = vm->byc->code[vm->pc];
	switch( BYTECODE_CMD40(byc) ){
		case OP_CHAR:
			if( *vm->sp == BYTECODE_VAL08(byc) ){
				++vm->sp;
			}
			else{
				on_error(vm);
				return stk_pop(vm);
			}
		break;
		
		case OP_RANGE:
			if( op_range(vm, BYTECODE_VAL12(byc), *vm->sp) ){
				++vm->sp;
			}
			else{
				on_error(vm);
				return stk_pop(vm);
			}
		break;
		
		case OP_URANGE:
			//TODO
		break;
		
		case OP_PUSH: stk_push(vm, vm->pc + BYTECODE_IVAL11(byc)); break;
		
		case OP_PUSHR: 
			stk_push(vm, vm->pc + 1);
			vm->pc += BYTECODE_IVAL11(byc);
		return 1;
		
		case OP_JMP: vm->pc += BYTECODE_IVAL11(byc); return 1;
		case OP_CALL: op_call(vm, BYTECODE_VAL12(byc)); return 1;
		case OP_NODE: op_node(vm, OPEV_NODEEX_NEW, BYTECODE_VAL12(byc)); break;
	
		case OP_ENTER:
			vm->ip = lips_ast_child_id(vm->ip, BYTECODE_VAL12(byc));
			if( !vm->ip ) return stk_pop(vm);
		break;
		
		case OP_TYPE: vm->ip->id = BYTECODE_VAL12(byc); break;
		
		case OP_SYMBOL:
			if( !scope_symbol_search_content(vm->sc, BYTECODE_VAL12(byc), vm->ip) ) return stk_pop(vm);
		break;
		
		case OP_EXT:
			switch(BYTECODE_CMD04(byc)){
				case OPE_MATCH : return BYTECODE_VAL08(byc);
				case OPE_SAVE  : op_save(vm, BYTECODE_VAL08(byc)); break;
				case OPE_RET   : return op_ret(vm, BYTECODE_VAL08(byc));
				case OPE_NODEEX: op_node(vm, BYTECODE_VAL08(byc), 0); break;
				case OPE_VALUE : return op_value(vm, BYTECODE_VAL08(byc));
				case OPE_SCOPE : op_scope(vm, BYTECODE_VAL08(byc)); break;
				case OPE_ERROR : op_error(vm, BYTECODE_VAL08(byc)); break;
				case OPE_LEAVE :
					vm->ip = vm->ip->parent;
					if( !vm->ip ) return stk_pop(vm);
				break;
			}
		break;
	}
	
	++vm->pc;
	return 1;
}

int lips_vm_match(lipsVM_s* vm){
	while( lips_vm_exec(vm) > 0 );
	if( vm->match->count ) ++vm->match->count;
	if( m_header(vm->node)->len ){
		vm->match->ast = lips_ast_make(vm->node);
		vm->sc         = vm->scope;
		mforeach(vm->byc->sfase, isf){
			mforeach(vm->match->ast.marked, in){
				mforeach(vm->byc->sfase[isf].addr, ia){
					lips_vm_semantic_reset(vm);
					vm->pc = vm->byc->sfase[isf].addr[ia];
					vm->ip = vm->match->ast.marked[in];
					while( lips_vm_exec(vm) > 0 );
				}
			}
		}
	}
	return vm->match->count;
}

int lips_range_test(lipsVM_s* vm, unsigned ir, utf8_t ch ){
	return op_range(vm, ir, ch);
}





















