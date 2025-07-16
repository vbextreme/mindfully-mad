#include <lips/bytecode.h>
#include <lips/debug.h>
#include <inutility.h>

typedef struct lipsVMDebug{
	termMultiSurface_s tms;
	lipsVM_s*          vm;
	termSurface_s*     header;
	termSurface_s*     stack;
}lipsVMDebug_s;


__private void d_create_surface(lipsVMDebug_s* d){
	dbg_info("");
	termMultiSurface_s* m = term_multi_surface_ctor(&d->tms, LIPS_DBG_X, LIPS_DBG_Y, LIPS_DBG_SCREEN_W);
	term_multi_surface_vsplit(m, LIPS_DBG_HEADER_H);
	term_multi_surface_hsplit(m, LIPS_DBG_HEADER_W);
	term_multi_surface_vsplit(m, LIPS_DBG_STACKED_H);
	term_multi_surface_hsplit(m, LIPS_DBG_STACK_W);
	term_multi_surface_hsplit(m, LIPS_DBG_CSTACK_W);
	term_multi_surface_hsplit(m, LIPS_DBG_NODE_W);
	term_multi_surface_hsplit(m, LIPS_DBG_CAPTURE_W);
	term_multi_surface_vsplit(m, LIPS_DBG_RANGE_H);
	term_multi_surface_hsplit(m, LIPS_DBG_RANGE_W);
	term_multi_surface_vsplit(m, LIPS_DBG_CODE_H);
	term_multi_surface_hsplit(m, LIPS_DBG_CODE_W);
	term_multi_surface_vsplit(m, LIPS_DBG_PROMPT_H);
	term_multi_surface_hsplit(m, LIPS_DBG_PROMPT_W);
	term_multi_surface_vsplit(m, LIPS_DBG_INP_H);
	term_multi_surface_hsplit(m, LIPS_DBG_INP_W);
	term_multi_surface_apply(m);
	d->header = &m->line[0].surface[0];
	d->stack  = &m->line[1].surface[0];
}

__private void draw_step(lipsVMDebug_s* d){
	unsigned y,ny,count,sc;
	//draw header
	term_surface_clear(d->header);
	term_surface_focus(d->header);
	printf("start:%04X ranges:%u functions:%u stacksize:%u", d->vm->byc->start, d->vm->byc->rangeCount, d->vm->byc->fnCount, m_header(d->vm->stack)->len);
	//draw stack
	term_surface_clear(d->stack);
	ny    = d->stack->h - 2;
	y     = d->stack->y+ny;
	count = m_header(d->vm->stack)->len;
	sc = count > ny ? count - ny: 0;
	while( ny-->0 && sc < count){
		term_gotoxy(1,y--);
		printf(" %% pc:%04X sp:%4lu ls:%3d",
			d->vm->stack[sc].pc,
			d->vm->stack[sc].sp-d->vm->txt,
			d->vm->stack[sc].ls
		);
		++sc;
	}

}
/*
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
*/
void lips_vm_debug(lipsVM_s* vm){
	dbg_info("");
	lipsVMDebug_s d;
	d_create_surface(&d);
	d.vm = vm;
	term_cls();
	term_multi_surface_draw(&d.tms);
	draw_step(&d);
	term_flush();
}


void lips_dump_error(lipsMatch_s* m, const utf8_t* source, FILE* f){
	if( m->count < 0 ){
		fprintf(f, "lips error: internal error, this never append for now\n");
	}
	else if( m->count > 0 ){
		fprintf(f, "lips error: no errors, lips have match\n");
	}
	else if( m->err.number ){
		fprintf(f, "lips fail: %u\n", m->err.number);
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
	else{
		fprintf(f, "lips error: match not have mark any type of error but not have match\n");
	}
	fflush(f);
}




