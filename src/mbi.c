#include <notstd/opt.h>
#include <fcntl.h>
#include <inutility.h>
#include <lips/vm.h>
#include <lips/builtingram.h>
#include <lips/bytecode.h>
#include <lips/debug.h>

//TODO
//dbginput scorre male
//dump ast, visualizzare il contenuto in una linea singola, quindi sovrascrivere \t\n


typedef enum{
	OPT_g,
	OPT_s,
	OPT_d,
	OPT_dump_capture,
	OPT_dump_ast_file,
	OPT_dump_ast_dot,
	OPT_h
}options_e;

option_s opt[] = {
	{ 'g', "--grammar"      , "grammar file"                                           ,              OPT_PATH | OPT_EXISTS, 0, NULL},
	{ 's', "--source"       , "source file"                                            ,  OPT_ARRAY | OPT_PATH | OPT_EXISTS, 0, NULL},
	{ 'd', "--debug"        , "debug"                                                  ,                          OPT_NOARG, 0, NULL},
	{'\0', "--dump-capture" , "dump capture to file, pass stdout for write on terminal",                           OPT_PATH, 0, NULL},
	{'\0', "--dump-ast-file", "dump ast to file, pass stdout for write on terminal"    ,                           OPT_PATH, 0, NULL},
	{'\0', "--dump-ast-dot" , "dump ast in dot format to a file"                       ,                           OPT_PATH, 0, NULL},
	{ 'h', "--help"         , "display this"                                           ,                OPT_NOARG | OPT_END, 0, NULL},
};

__private FILE* argfopen(const char* path, const char* mode){
	if( !strcmp(path, "stdout") ) return stdout;
	if( !strcmp(path, "stderr") ) return stderr;
	dbg_info("OPEN %s", path);
	FILE* ret = fopen(path, mode);
	if( !ret ) die("unable to open file '%s': %m", path);
	return ret;
}

__private void argfclose(FILE* f){
	if( f == stdout ) return;
	if( f == stderr ) return;
	if( !f ) return;
	fclose(f);
}

int main(int argc, char** argv){
	argv_parse(opt, argc, argv);
	if( opt[OPT_h].set ){
		argv_usage(opt, argv[0]);
		return 0;
	}
	if( !opt[OPT_s].set ) die("required argument --source");
	
	if( opt[OPT_dump_capture].set && m_header(opt[OPT_dump_capture].value)->len != m_header(opt[OPT_s].value)->len ){
		die("for dump capture need pass many file same many source have passed, for example -s A,B --dump-caputre stdout,stdout");
	}
	if( opt[OPT_dump_ast_file].set && m_header(opt[OPT_dump_ast_file].value)->len != m_header(opt[OPT_s].value)->len ){
		die("for dump ast need pass many file same many source have passed, for example -s A,B --dump-ast-file cA,cB");
	}
	if( opt[OPT_dump_ast_dot].set && m_header(opt[OPT_dump_ast_dot].value)->len != m_header(opt[OPT_s].value)->len ){
		die("for dump ast need pass many file same many source have passed, for example -s A,B --dump-ast-dot cA,cB");
	}
	
	uint16_t* lgram = lips_builtin_grammar_make();
	if( !lgram ) die("internal error, wrong builtin lips grammar");
	lipsByc_s lbyc;
	lipsByc_ctor(&lbyc, lgram);
	lipsVM_s vm;
	lips_vm_ctor(&vm, &lbyc);
	
	if( opt[OPT_g].set ){
		lipsMatch_s m;
		__free utf8_t* source = (utf8_t*)load_file(opt[OPT_g].value->str);
		lips_vm_reset(&vm, &m, source);
		if( lips_vm_match(&vm) < 1 ){
			lips_dump_error(&m, source, stderr);
			die("");
		}
		die("TODO extract new grammar");
		lipsByc_dtor(&lbyc);
		lips_vm_dtor(&vm);
		lipsByc_ctor(&lbyc, lgram);
		lips_vm_ctor(&vm, &lbyc);
	}

	mforeach(opt[OPT_s].value, it){
		lipsMatch_s m;
		__free utf8_t* source = (utf8_t*)load_file(opt[OPT_s].value[it].str);
		lips_vm_reset(&vm, &m, source);
		
		if( opt[OPT_d].set ){
			lips_vm_debug(&vm);
		}
		else{
			if( lips_vm_match(&vm) < 1 ){
				lips_dump_error(&m, source, stderr);
				die("");
			}
		}
		die("");
		if( opt[OPT_dump_capture].set ){
			FILE* out = argfopen(opt[OPT_dump_capture].value[it].str, "w");
			//lips_dump_capture(&m, out);
			argfclose(out);
		}
		if( opt[OPT_dump_ast_file].set ){
			FILE* out = argfopen(opt[OPT_dump_ast_file].value[it].str, "w");
			//lips_dump_ast(&m, grambyc, out, 1, 0);
			argfclose(out);
		}
		if( opt[OPT_dump_ast_dot].set ){
			FILE* out = argfopen(opt[OPT_dump_ast_dot].value[it].str, "w");
			//lips_dump_ast(&m, grambyc, out, 0, 1);
			argfclose(out);
		}
	}
	
	lips_vm_dtor(&vm);
	lipsByc_dtor(&lbyc);
	return 0;
}

