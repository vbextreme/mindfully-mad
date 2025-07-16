#include <lips/bytecode.h>
#include <lips/debug.h>
#include <inutility.h>

typedef struct lipsVMDebug{
	termMultiSurface_s tms;
	lipsVM_s*          vm;
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
}

void lips_vm_debug(lipsVM_s* vm){
	dbg_info("");
	lipsVMDebug_s d;
	d_create_surface(&d);
	d.vm = vm;
	term_cls();
	term_multi_surface_draw(&d.tms);
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




