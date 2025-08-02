#include <notstd/str.h>
#include <lips/bytecode.h>
#include <lips/debug.h>
#include <inutility.h>
#include <readline/readline.h>

//TODO
//input draw overflow
//code draw overflow
//debug splith code with ast

typedef enum { 
	DBG_STATE_QUIT = -1,
	DBG_STATE_STEP,
	DBG_STATE_BREAK,
	DBG_STATE_RUN
}dbgState_e;

typedef struct cmdInput{
	char*  line;
	char** argv;
}cmdInput_s;

typedef struct lipsVMDebug{
	termMultiSurface_s tms;
	lipsVM_s*          vm;
	termSurface_s*     header;
	termSurface_s*     stack;
	termSurface_s*     cstack;
	termSurface_s*     node;
	termSurface_s*     save;
	termSurface_s*     brk;
	termSurface_s*     range;
	termSurface_s*     input;
	termSurface_s*     code;
	termSurface_s*     prompt;
	cmdInput_s         cinp;
	unsigned           txtLen;
	unsigned           curStack;
//	unsigned           curPC;
	unsigned           stackLen;
	uint32_t*          breakpoint;
	unsigned           brrm;
	long               brtxt;
	unsigned           brsem;
	dbgState_e         state;
}lipsVMDebug_s;

typedef void(*action_f)(lipsVMDebug_s* d);

typedef struct cmdmap{
	const char* shr;
	const char* lng;
	action_f    act;
	unsigned    nam;
}cmdmap_s;


typedef void (*drawrevline_f)(lipsVMDebug_s* d, unsigned i);

__private void d_create_surface(lipsVMDebug_s* d){
	termMultiSurface_s* m = term_multi_surface_ctor(&d->tms, LIPS_DBG_X, LIPS_DBG_Y, LIPS_DBG_SCREEN_W);
	term_multi_surface_vsplit(m, LIPS_DBG_HEADER_H);
	term_multi_surface_hsplit(m, " lips debug - header ", LIPS_DBG_HEADER_W);
	term_multi_surface_vsplit(m, LIPS_DBG_STACKED_H);
	term_multi_surface_hsplit(m, " stack ", LIPS_DBG_STACK_W);
	term_multi_surface_hsplit(m, " call ", LIPS_DBG_CSTACK_W);
	term_multi_surface_hsplit(m, " node ", LIPS_DBG_NODE_W);
	term_multi_surface_hsplit(m, " capture ", LIPS_DBG_CAPTURE_W);
	term_multi_surface_hsplit(m, " break ", LIPS_DBG_BREAK_W);
	term_multi_surface_vsplit(m, LIPS_DBG_RANGE_H);
	term_multi_surface_hsplit(m, " range ", LIPS_DBG_RANGE_W);
	term_multi_surface_vsplit(m, LIPS_DBG_CODE_H);
	term_multi_surface_hsplit(m, " code ", LIPS_DBG_CODE_W);
	term_multi_surface_vsplit(m, LIPS_DBG_PROMPT_H);
	term_multi_surface_hsplit(m, " prompt ", LIPS_DBG_PROMPT_W);
	term_multi_surface_vsplit(m, LIPS_DBG_INP_H);
	term_multi_surface_hsplit(m, " input ", LIPS_DBG_INP_W);
	term_multi_surface_apply(m);
	d->header = &m->line[0].surface[0];
	d->stack  = &m->line[1].surface[0];
	d->cstack = &m->line[1].surface[1];
	d->node   = &m->line[1].surface[2];
	d->save   = &m->line[1].surface[3];
	d->brk    = &m->line[1].surface[4];
	d->range  = &m->line[2].surface[0];
	d->code   = &m->line[3].surface[0];
	d->prompt = &m->line[4].surface[0];
	d->input  = &m->line[5].surface[0];
}

__private void reverse_draw(lipsVMDebug_s* d, termSurface_s* s, unsigned count, drawrevline_f fn){
	term_surface_clear(s);
	unsigned ny    = s->h - 2;
	unsigned y     = s->y+ny;
	unsigned sc = count > ny ? count - ny: 0;
	while( ny-->0 && sc < count){
		term_gotoxy(s->x+1,y--);
		fn(d, sc);
		++sc;
	}
}

__private void revdraw_stack(lipsVMDebug_s* d, unsigned sc){
	printf(" %% pc:%04X tp:%4lu ls:%3d",
		d->vm->stack[sc].pc,
		d->vm->stack[sc].sp-d->vm->txt,
		d->vm->stack[sc].ls
	);
	if( d->vm->stack[sc].ip ){
		printf(" ip:%3X", d->vm->stack[sc].ip->id);
	}
}

__private void revdraw_cstack(lipsVMDebug_s* d, unsigned sc){
	printf(" <- sp:%4X", d->vm->cstk[sc]);
}

__private void revdraw_node(lipsVMDebug_s* d, unsigned sc){
	switch( d->vm->node[sc].op ){
		case OPEV_NODEEX_NEW    : printf(" + [%4u](%3d)%s len:%lu", sc, d->vm->node[sc].id, d->vm->byc->name[d->vm->node[sc].id], d->vm->node[sc].sp - d->vm->txt); break;
		case OPEV_NODEEX_PARENT : printf(" < [%4d] %lu", sc, d->vm->node[sc].sp - d->vm->txt); break;
		case OPEV_NODEEX_DISABLE: printf(" ~ [%4u]", sc); break;
		case OPEV_NODEEX_ENABLE : printf(" & [%4u]", sc); break;
		default: printf(" INTERNAL ERROR ");
	}
}

__private void revdraw_save(lipsVMDebug_s* d, unsigned sc){
	printf(" $(%3u) %6ld", sc, d->vm->match->capture[sc] ? d->vm->match->capture[sc] - d->vm->txt : -1);
}

__private void revdraw_breakpoint(lipsVMDebug_s* d, unsigned sc){
	printf(" X %4X", d->breakpoint[sc]);
}

__private void draw_range(lipsVMDebug_s* d, unsigned idrange, long ch){
	const unsigned cunset = 130;
	const unsigned cerr   = 124;
	const unsigned cset   = 22;
	const unsigned cmatch = 17;
	term_gotoxy(d->range->x+1, d->range->y+1);
	printf("range:%4u ch:", idrange);
	if( ch == -1 ){
		term_bcol(cunset);
	}
	else{
		term_bcol( lips_range_test(d->vm, idrange, ch)? cmatch: cerr);
	}
	fputs(cast_view_char(ch, 0), stdout);
	term_reset();
	printf("  ");
	for( unsigned i = '\t'; i < 128; ++i ){
		if( i != '\t' && i != '\n' && i < 32 ) continue;
		if( lips_range_test(d->vm, idrange, i ) ){
			if( ch == i ) term_bcol(cmatch);
			else term_bcol(cset);
		}
		else{
			if( ch == i ) term_bcol(cerr);
			else term_bcol(cunset);
		}
		fputs(cast_view_char(i, 0), stdout);
	}
	term_reset();
}

__private void draw_input(lipsVMDebug_s* d, int oncol){
	term_surface_clear(d->input);
	//unsigned const ls = d->curStack;
	//unsigned offx = d->vm->stack[ls].sp - d->vm->txt;
	unsigned offx = d->vm->sp - d->vm->txt;

	long low = offx;
	low -= (d->input->w-2)/2;
	if( low < 0 ) low = 0;
	unsigned high = low + d->input->w-2;
	if( high > d->txtLen ){
		high = d->txtLen;
		low = (long)high - (d->input->w-2);
		if( low < 0 ) low = 0;
	}
	term_gotoxy(d->input->x+1, d->input->y+1);
	for( unsigned i = low; i < high; ++i ){
//		if( &d->vm->txt[i] == d->vm->stack[ls].sp ){
		if( &d->vm->txt[i] == d->vm->sp ){
			switch( oncol ){
				case 0: term_fcol(160); break;
				case 1: term_fcol(46); break;
			}
			fputs(cast_view_char(d->vm->txt[i],0), stdout);
			term_reset();
			offx = i-low;
		}
		else{
			fputs(cast_view_char(d->vm->txt[i],0), stdout);
		}
	}
	term_gotoxy(d->input->x+offx+1, d->input->y+2);
	switch( oncol ){
		case 0: term_fcol(160); break;
		case 1: term_fcol(46); break;
	}
	putchar('^');
	term_reset();
}

__private unsigned draw_opcode(lipsVMDebug_s* d, unsigned pc){
	unsigned byc = d->vm->byc->code[pc];
	switch( BYTECODE_CMD40(byc) ){
		default : printf("INTERNAL ERROR CMD40: 0x%X", BYTECODE_CMD40(byc)); break;
		case OP_RANGE :
			printf("range %u", BYTECODE_VAL12(byc));
//			if( d->curPC == pc ){
			if( d->vm->pc == pc ){
				term_reset();
				draw_range(d, BYTECODE_VAL12(byc), *d->vm->stack[d->curStack].sp);
				draw_input(d, !!lips_range_test(d->vm, BYTECODE_VAL12(byc), *d->vm->stack[d->curStack].sp));
			}
		break;
		case OP_URANGE: break;
		case OP_CHAR  :
			fputs("char  ", stdout); fputs(cast_view_char(BYTECODE_VAL08(byc),1), stdout);
//			if( d->curPC == pc ){
			if( d->vm->pc == pc ){
				term_reset();
				draw_input(d, BYTECODE_VAL08(byc) == *d->vm->stack[d->curStack].sp);
			}
		break;
		case OP_PUSH  : printf("push  %X -> pc %X", pc + BYTECODE_IVAL11(byc), pc+1); break;
		case OP_PUSHR : printf("push  %X -> pc %X", pc + 1, pc + BYTECODE_IVAL11(byc)); break;
		case OP_JMP   : printf("jmp   %X", pc + BYTECODE_IVAL11(byc)); break;
		case OP_NODE  : printf("node  new::%s::%u", d->vm->byc->name[BYTECODE_VAL12(byc)],BYTECODE_VAL12(byc)); break;
		case OP_CALL  : printf("call  [%u]%s -> %X",BYTECODE_VAL12(byc), d->vm->byc->name[BYTECODE_VAL12(byc)], d->vm->byc->fn[BYTECODE_VAL12(byc)]); break;
		case OP_ENTER : printf("enter %X",BYTECODE_VAL12(byc)); break;
		case OP_TYPE  : printf("type  %X",BYTECODE_VAL12(byc)); break;
		case OP_SYMBOL: printf("symbol %X",BYTECODE_VAL12(byc)); break;

		case OP_EXT:
			switch(BYTECODE_CMD04(byc)){
				case OPE_MATCH : printf("match %u", BYTECODE_VAL08(byc)); break;
				case OPE_SAVE  : printf("save  %u", BYTECODE_VAL08(byc)); break;
				case OPE_RET   : printf("ret   %u", BYTECODE_VAL08(byc)); break;
				case OPE_NODEEX:
					switch( BYTECODE_VAL08(byc) ){
						case OPEV_NODEEX_PARENT : printf("node  parent"); break;
						case OPEV_NODEEX_DISABLE: printf("node  disable"); break;
						case OPEV_NODEEX_ENABLE : printf("node  enable"); break;
						case OPEV_NODEEX_MARK   : printf("node  mark"); break;
					}
				break;
				case OPE_LEAVE: printf("leave"); break;
				case OPE_VALUE:{
					unsigned len;
					uint32_t next;
					const char* str = lipsByc_extract_string(d->vm->byc->bytecode, pc, &next, &len);
					switch(BYTECODE_VAL08(byc)){
						default: printf("INTERNAL ERROR OPEV_VALUE CMD08 0x%X", BYTECODE_VAL08(byc)); break;
						case OPEV_VALUE_SET:
							printf("value set, '%s'", str);
						break;
						case OPEV_VALUE_TEST:
							printf("value test, '%s'", str);
						break;
					}
					return next-pc;
				}
				case OPE_SCOPE:
					switch(BYTECODE_VAL08(byc)){
						default: printf("INTERNAL ERROR OPEV_VALUE CMD08 0x%X", BYTECODE_VAL08(byc)); break;
						case OPEV_SCOPE_NEW   : printf("scope new"); break;
						case OPEV_SCOPE_LEAVE : printf("scope leave"); break;
						case OPEV_SCOPE_SYMBOL: printf("scope symbol"); break;
					}
				break;
				case OPE_ERROR: printf("error %u", BYTECODE_VAL08(byc)); break;
				default: printf("INTERNAL ERROR CMD04: 0x%X", BYTECODE_CMD04(byc)); break;
			}
		break;
	}
	return 1;
}

__private void draw_code(lipsVMDebug_s* d){
	unsigned const count = d->vm->byc->codeLen;
	unsigned const csel  = 63;
	term_surface_clear(d->code);
	long low  = d->vm->pc;//d->curPC;
   	low -= (d->code->h-2)/2;
	if( low < 0 ) low = 0;
	unsigned high = low + d->code->h-2;
	if( high > count ){
		high = count;
		low = (long)high - (d->code->h-2);
		if( low < 0 ) low = 0;
	}
	unsigned y = d->code->y+1;
	for(unsigned i = low; i < high; ++i ){
		term_gotoxy(d->code->x+1, y++);
		//if( d->curPC == i ){
		if( d->vm->pc == i ){
			term_fcol(csel);
			printf(">");
		}
		else{
			printf(" ");
		}
		printf("%04X  ", i);
		unsigned n = draw_opcode(d, i);
		if( n > 1 ){
			high += n;
			i += n-1;
		}
		term_reset();
	}
}

__private void draw_step(lipsVMDebug_s* d){
	term_surface_clear(d->header);
	term_surface_focus(d->header);
	term_flush();
	printf("start:%04X ranges:%u functions:%u codesize: %X stacksize:%u rerr:%u",
			d->vm->byc->start,
			d->vm->byc->rangeCount,
			d->vm->byc->fnCount,
			d->vm->byc->codeLen,
			d->stackLen,
			d->vm->er
	);
	if( d->vm->ip ) printf(" ip:%3X", d->vm->ip->id);
	reverse_draw(d, d->stack, m_header(d->vm->stack)->len, revdraw_stack);
	reverse_draw(d, d->cstack, m_header(d->vm->cstk)->len, revdraw_cstack);
	reverse_draw(d, d->node, m_header(d->vm->node)->len, revdraw_node);
	reverse_draw(d, d->save, d->vm->match->count+1, revdraw_save);
	reverse_draw(d, d->brk, m_header(d->breakpoint)->len, revdraw_breakpoint);
	term_flush();
	term_surface_clear(d->range);
	draw_input(d, -1);
	draw_code(d);
	term_flush();
}

__private void prompt(lipsVMDebug_s* d){
	term_surface_clear(d->prompt);
	term_surface_focus(d->prompt);
	free(d->cinp.line);
	m_free(d->cinp.argv);
	term_flush();
	d->cinp.line = readline("> ");
	d->cinp.argv = str_splitin(d->cinp.line, " ", 0);
	if( m_header(d->cinp.argv)->len == 0 ){
		unsigned i = m_ipush(&d->cinp.argv);
		d->cinp.argv[i] = d->cinp.line;
	}
}

__private int pccmp(const void* a, const void* b){
	return *(uint32_t*)a - *(uint32_t*)b;
}

__private long breakpoint_find(lipsVMDebug_s* d, uint32_t pc){
	return m_ibsearch(d->breakpoint, &pc, pccmp);
}

__private unsigned breakpoint_add(lipsVMDebug_s* d, uint32_t pc){
	if( breakpoint_find(d, pc) == -1 && pc < d->vm->byc->codeLen ){
		unsigned i = m_ipush(&d->breakpoint);
		d->breakpoint[i] = pc;
		m_qsort(d->breakpoint, pccmp);
		reverse_draw(d, d->brk, m_header(d->breakpoint)->len, revdraw_breakpoint);
		return 1;
	}
	return 0;
}

__private void breakpoint_rm(lipsVMDebug_s* d, uint32_t pc){
	long id = breakpoint_find(d, pc);
	if( id < 0 ) return;
	m_delete(d->breakpoint, id, 1);
	reverse_draw(d, d->brk, m_header(d->breakpoint)->len, revdraw_breakpoint);
}

__private void action_nop(__unused lipsVMDebug_s* l){}

__private void action_step(lipsVMDebug_s* d){
	d->state = DBG_STATE_STEP;
}

__private void action_next(lipsVMDebug_s* d){
	const unsigned pc  = d->vm->pc;
	const unsigned byc = d->vm->byc->code[pc];
	if( BYTECODE_CMD40(byc) == OP_CALL ){
		d->brrm = 0;
		d->brrm += breakpoint_add(d, pc+1);
		if( d->curStack ){
			d->brrm += breakpoint_add(d, d->vm->stack[d->curStack-1].pc);
		}
		d->state = DBG_STATE_RUN;
	}
	else{
		d->state = DBG_STATE_STEP;
	}
}

__private void action_continue(lipsVMDebug_s* d){
	d->state = DBG_STATE_RUN;
}

__private void action_quit(lipsVMDebug_s* d){
	d->state = DBG_STATE_QUIT;
}

__private void action_breakpoint(lipsVMDebug_s* d){
	breakpoint_add(d, strtoul(d->cinp.argv[1], NULL, 16));
}

__private void action_breaktext(lipsVMDebug_s* d){
	d->brtxt = strtoul(d->cinp.argv[1], NULL, 10);
	d->state = DBG_STATE_RUN;
}

__private void action_breaksemantic(lipsVMDebug_s* d){
	d->brsem = 1;
	d->state = DBG_STATE_RUN;
}

__private void action_breakremove(lipsVMDebug_s* d){
	breakpoint_rm(d, strtoul(d->cinp.argv[1], NULL, 16));
}

__private void action_refresh(lipsVMDebug_s* d){
	term_multi_surface_draw(&d->tms);
	draw_step(d);
}

cmdmap_s CMD[] = {{
		.shr = "",
		.lng = "",
		.act = action_nop,
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
		.shr = "br",
		.lng = "breakremove",
		.act = action_breakremove,
		.nam = 1
	},{
		.shr = "bt",
		.lng = "breaktext",
		.act = action_breaktext,
		.nam = 1
	},{
		.shr = "bs",
		.lng = "breaksemantic",
		.act = action_breaksemantic,
		.nam = 0
	},{
		.shr = "r",
		.lng = "refresh",
		.act = action_refresh,
		.nam = 0
	}	
};

__private void command_parse(lipsVMDebug_s* d){
	for( unsigned i = 0; i < sizeof_vector(CMD); ++i ){
		if( !strcmp(d->cinp.argv[0], CMD[i].shr) || !strcmp(d->cinp.argv[0], CMD[i].lng) ){
			if( CMD[i].nam == m_header(d->cinp.argv)->len-1 ) CMD[i].act(d);
			if( !CMD[i].nam )  CMD[0].act = CMD[i].act;
			return;
		}
	}
}

__private dbgState_e debug_exec(lipsVMDebug_s* d){
	if( d->state == DBG_STATE_QUIT ) return DBG_STATE_QUIT;
	d->stackLen = m_header(d->vm->stack)->len;
	d->curStack = d->stackLen ? d->stackLen - 1: 0;
	draw_step(d);
	if( breakpoint_find(d, d->vm->pc) != -1 ) d->state = DBG_STATE_BREAK;
	if( d->brtxt == d->vm->sp - d->vm->txt ){
		d->brtxt = -1;
		d->state = DBG_STATE_BREAK;
	}
	if( d->state == DBG_STATE_STEP ) d->state = DBG_STATE_BREAK;
	while( d->state == DBG_STATE_BREAK ){
		if( d->brrm ){
			iassert(m_header(d->breakpoint)->len >= d->brrm);
			m_header(d->breakpoint)->len -= d->brrm;
			d->brrm = 0;
		}
		prompt(d);
		command_parse(d);
	}
	return d->state;
}

int lips_vm_match_debug(lipsVM_s* vm){
	int ret = 0;
	dbg_info("");
	lipsVMDebug_s d;
	d.txtLen = strlen((char*)vm->txt);
	d_create_surface(&d);
	d.vm = vm;
	d.cinp.line  = NULL;
	d.cinp.argv  = NULL;
	d.state      = DBG_STATE_BREAK;
	d.brrm       = 0;
	d.brtxt      = -1;
	d.brsem      = 0;
	d.breakpoint = MANY(uint32_t, 16);
	term_cls();
	term_multi_surface_draw(&d.tms);
	term_flush();
	while( debug_exec(&d) != DBG_STATE_QUIT && lips_vm_exec(d.vm) > 0 );
	ret = vm->match->count;
	if( d.brsem && d.state != DBG_STATE_QUIT ) d.state = DBG_STATE_STEP;
	if( m_header(vm->node)->len ){
		vm->match->ast = lips_ast_make(vm->node);
		vm->sc         = vm->scope;
		mforeach(vm->byc->sfase, isf){
			mforeach(vm->match->ast.marked, in){
				mforeach(vm->byc->sfase[isf].addr, ia){
					lips_vm_semantic_reset(vm);
					vm->pc = vm->byc->sfase[isf].addr[ia];
					vm->ip = vm->match->ast.marked[in];
					int test = 0;
					while( debug_exec(&d) != DBG_STATE_QUIT && (test=lips_vm_exec(d.vm))>0 );
					if( test < 0 ) {
						ret = 0;
						goto BREAKOUT;
					}
				}
			}
		}
	}
BREAKOUT:
	term_cls();
	term_flush();
	return ret;
}

void lips_dump_error(lipsVM_s* vm, lipsMatch_s* m, const utf8_t* source, FILE* f){
	dbg_info("count: %u loc: %p", m->count, m->err.loc);
	if( m->count < 0 ){
		fprintf(f, "lips error: internal error, this never append for now\n");
	}
	else if( !m->count && m->err.loc ){
		fprintf(f, "lips fail(%u) %s\n", m->err.number, vm->byc->errStr[m->err.number]);
		const char*    sp    = (const char*)m->err.loc;
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
	fflush(f);
}

void lips_dump_capture(lipsMatch_s* m, FILE* f){
	if( m->count < 0 ){
		fputs("lips capture: error, this never append for now\n", f);
		return;
	}
	for( int i = 0; i < m->count/2; ++i ){
		unsigned st = i*2;
		unsigned en = st+1;
		unsigned len = m->capture[en] - m->capture[st];
		fprintf(f, "<capture%u>%.*s</capture%u>\n", i, len, m->capture[st], i);
	}
}

__private void ast_dump_file(lipsAst_s* ast, const char** nmap, unsigned tab, FILE* f){
	for( unsigned i = 0; i < tab; ++i ){
		fputs("  ", f);
	}
	if( m_header(ast->child)->len ){
		fprintf(f, "<%s>\n", ast->id == LIPS_NODE_START ? "_start_": nmap[ast->id]);
		mforeach(ast->child,i){
			ast_dump_file(ast->child[i], nmap, tab+1, f);
		}
	}
	else{
		fprintf(f, "<%s> '", ast->id == LIPS_NODE_START ? "_start_": nmap[ast->id]);
		for( unsigned i = 0; i < ast->len; ++i ){
			fputs(cast_view_char(ast->tp[i], 0), f);
		}
		fputs("'\n", f);
	}
}

__private void dump_dot(lipsAst_s* ast, const char** nmap, FILE* f, const char* parent, unsigned index){
	__free char* name = str_printf("%s_%s_%d", parent, ast->id == LIPS_NODE_START ? "_start_": nmap[ast->id], index);
	if( *parent ) fprintf(f, "%s -> %s;\n", parent, name);

	if( m_header(ast->child)->len ){
		fprintf(f, "%s [label=\"%s\"];\n", name, ast->id == LIPS_NODE_START ? "_start_": nmap[ast->id]);
		mforeach(ast->child,i){
			dump_dot(ast->child[i], nmap, f, name, i);
		}
	}
	else{
		fprintf(f, "%s [label=\"%s\\n", name, ast->id == LIPS_NODE_START ? "_start_": nmap[ast->id]);
		for( unsigned i = 0; i < ast->len; ++i ){
			fputs(cast_view_char(ast->tp[i], 0), f);
		}
		fprintf(f, "\"];\n");
	}
}

__private void ast_dump_dot(lipsAst_s* ast, const char** nmap, FILE* f){
	fputs("digraph {\n", f);
	dump_dot(ast, nmap, f, "", 0);
	fputs("}\n", f);
}

void lips_dump_ast(lipsVM_s* vm, lipsAst_s* root, FILE* f, int mode){
	dbg_info("dump %p", root);
	if( !root ) return;
	switch( mode ){
		case 0: ast_dump_file(root, vm->byc->name, 0, f); break;
		case 1: ast_dump_dot(root, vm->byc->name, f); break;
	}
	fflush(f);
}

void lips_dump_name_cenum(lipsByc_s* byc, const char* name, const char* prefix, FILE* f){
	fprintf(f, "typedef enum{\n");
	mforeach(byc->name, i){
		fprintf(f, "\t%s_%s,\n", prefix, byc->name[i]);
	}
	fprintf(f, "}%s_e;\n", name);
}


















