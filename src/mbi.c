#include <mfm.h>
#include <notstd/opt.h>
#include <unistd.h>
#include <fcntl.h>
#include <mfmrevm.h>
#include <inutility.h>

//TODO
//dbginput scorre male
//dump ast, visualizzare il contenuto in una linea singola, quindi sovrascrivere \t\n


typedef enum{
	OPT_g,
	OPT_reasm_gram,
	OPT_s,
	OPT_d,
	OPT_dump_capture,
	OPT_dump_ast_file,
	OPT_dump_ast_dot,
	OPT_h
}options_e;

option_s opt[] = {
	{ 'g', "--grammar"      , "grammar file"                                           ,              OPT_PATH | OPT_EXISTS, 0, NULL},
	{'\0', "--lipsasm-gram" , "use builtin lips asm grammar"                           ,                          OPT_NOARG, 0, NULL},
	{ 's', "--source"       , "source file"                                            ,  OPT_ARRAY | OPT_PATH | OPT_EXISTS, 0, NULL},
	{ 'd', "--debug"        , "debug"                                                  ,                          OPT_NOARG, 0, NULL},
	{'\0', "--dump-capture" , "dump capture to file, pass stdout for write on terminal",                          OPT_PATH , 0, NULL},
	{'\0', "--dump-ast-file", "dump ast to file, pass stdout for write on terminal"    ,                          OPT_PATH , 0, NULL},
	{'\0', "--dump-ast-dot" , "dump ast in dot format to a file"                       ,                          OPT_PATH , 0, NULL},
	{ 'h', "--help"         , "display this"                                           ,                OPT_NOARG | OPT_END, 0, NULL},
};

int main(int argc, char** argv){
	__free uint16_t* grambyc = NULL;

	argv_parse(opt, argc, argv);
	if( opt[OPT_h].set ){
		argv_usage(opt, argv[0]);
		return 0;
	}
	if( !opt[OPT_g].set ){
		if( opt[OPT_reasm_gram].set ){
			grambyc = reasmgram_make();
		}
		else{
			die("required argument --grammar");
		}
	}
	else{
		//TODO make grammar
	}
	if( !opt[OPT_s].set )die("required argument --source");
	
	//mfm_s mfm;
	//mfm_s_ctor(&mfm);
	//mfm_load_grammar(&mfm, opt[OPT_g].value->str);
	
	if( opt[OPT_dump_capture].set && m_header(opt[OPT_dump_capture].value)->len != m_header(opt[OPT_s].value)->len ){
		die("for dump capture need pass many file same many source have passed, for example -s A,B --dump-caputre stdout,stdout");
	}
	if( opt[OPT_dump_ast_file].set && m_header(opt[OPT_dump_ast_file].value)->len != m_header(opt[OPT_s].value)->len ){
		die("for dump ast need pass many file same many source have passed, for example -s A,B --dump-ast-file cA,cB");
	}
	if( opt[OPT_dump_ast_dot].set && m_header(opt[OPT_dump_ast_dot].value)->len != m_header(opt[OPT_s].value)->len ){
		die("for dump ast need pass many file same many source have passed, for example -s A,B --dump-ast-dot cA,cB");
	}

	mforeach(opt[OPT_s].value, it){
		lipsMatch_s m;
		__free utf8_t* source = (utf8_t*)load_file(opt[OPT_s].value[it].str);
	//	mfm_clear(&mfm);
	//	printf("<%s>\n", opt[OPT_s].value[it].str);
	//	mfm_lexer_file(&mfm, opt[OPT_s].value[it].str);
	//	if( opt[OPT_dump_token].set ) mfm_token_dump(&mfm);
		if( opt[OPT_d].set ){
			lips_debug(grambyc, source, &m);
		}
		else{
			lips_match(grambyc, source, &m);
		}
		if( opt[OPT_dump_capture].set ){
			FILE* out = strcmp(opt[OPT_dump_capture].value[it].str, "stdout") ? fopen(opt[OPT_dump_capture].value[it].str, "w"): stdout;
			if( !out ) die("unable to create file %s:%m", opt[OPT_dump_capture].value[it].str);
			lips_dump_capture(&m, out);
			if( out && out != stdout ) fclose(out);
		}
		if( opt[OPT_dump_ast_file].set ){
			FILE* out = strcmp(opt[OPT_dump_ast_file].value[it].str, "stdout") ? fopen(opt[OPT_dump_ast_file].value[it].str, "w"): stdout;
			if( !out ) die("unable to create file %s:%m", opt[OPT_dump_ast_file].value[it].str);
			lips_dump_ast(&m, grambyc, out, 1, 0);
			if( out && out != stdout ) fclose(out);
		}
		if( opt[OPT_dump_ast_dot].set ){
			FILE* out = strcmp(opt[OPT_dump_ast_dot].value[it].str, "stdout") ? fopen(opt[OPT_dump_ast_dot].value[it].str, "w"): stdout;
			if( !out ) die("unable to create file %s:%m", opt[OPT_dump_ast_dot].value[it].str);
			lips_dump_ast(&m, grambyc, out, 0, 1);
			if( out && out != stdout ) fclose(out);
		}
	}
	//mfm_s_dtor(&mfm);
}
/*

fn.dot: node new
	char '.'
	node parent
	ret

fn.float: call num
	call dot
	call num
	ret
*/
