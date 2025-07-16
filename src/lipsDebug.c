#include <lips/bytecode.h>
#include <lips/debug.h>
#include <inutility.h>

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




