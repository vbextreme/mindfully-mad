#include <notstd/opt.h>
#include <notstd/str.h>
#include <fcntl.h>
#include <inutility.h>
#include <lips/vm.h>
#include <lips/builtingram.h>
#include <lips/bytecode.h>
#include <lips/debug.h>

typedef enum{
	OPT_g,
	OPT_s,
	OPT_d,
	OPT_dump_capture,
	OPT_dump_ast_file,
	OPT_dump_ast_dot,
	OPT_dump_ast_split,
	OPT_dot_png,
	OPT_dump_name_cenum,
	OPT_builtin_lips_emitter,
	OPT_emitter_ccode,
	OPT_builtin_lips_objdump,
	OPT_h
}options_e;

option_s opt[] = {
	{ 'g', "--grammar"             , "grammar file"                                           ,              OPT_PATH | OPT_EXISTS, 0, NULL},
	{ 's', "--source"              , "source file"                                            ,  OPT_ARRAY | OPT_PATH | OPT_EXISTS, 0, NULL},
	{ 'd', "--debug"               , "debug"                                                  ,                          OPT_NOARG, 0, NULL},
	{'\0', "--dump-capture"        , "dump capture to file, pass stdout for write on terminal",                           OPT_PATH, 0, NULL},
	{'\0', "--dump-ast-file"       , "dump ast to file, pass stdout for write on terminal"    ,                           OPT_PATH, 0, NULL},
	{'\0', "--dump-ast-dot"        , "dump ast in dot format to a file"                       ,                           OPT_PATH, 0, NULL},
	{'\0', "--dump-ast-split"      , "dump ast splitted on first multi child"                 ,                          OPT_NOARG, 0, NULL},
	{'\0', "--dot-png"             , "auto build dot file to png"                             ,                          OPT_NOARG, 0, NULL},
	{'\0', "--dump-name-enum"      , "dump grammar name to C enum"                            ,                           OPT_PATH, 0, NULL},
	{'\0', "--builtin-lips-emitter", "build lips in bytecode"                                 ,                           OPT_PATH, 0, NULL},
	{'\0', "--emitter-ccode"       , "save emitter as c code, argument is var name"           ,                            OPT_STR, 0, NULL},
	{'\0', "--builtin-lips-objdump", "dump previous builded grammar"                          ,              OPT_EXISTS | OPT_PATH, 0, NULL},
	{ 'h', "--help"                , "display this"                                           ,                OPT_NOARG | OPT_END, 0, NULL},
};

__private FILE* argfopen(long id, const char* path, const char* mode){
	if( !strcmp(path, "stdout") ) return stdout;
	if( !strcmp(path, "stderr") ) return stderr;
	dbg_info("OPEN %ld.%s", id, path);
	__free char* p = id != -1 ? str_printf("%ld.%s", id, path): str_dup(path, 0);
	FILE* ret = fopen(p, mode);
	if( !ret ) die("unable to open file '%s': %m", path);
	return ret;
}

__private void argfclose(FILE* f){
	if( f == stdout ) return;
	if( f == stderr ) return;
	if( !f ) return;
	fclose(f);
}

__private void dotbuild(long id, const char* fname){
	__free char* cmd = id == -1? str_printf("dot -Tpng %s -o %s.png", fname, fname): str_printf("dot -Tpng %ld.%s -o %ld.%s.png", id, fname, id, fname);
	system(cmd);
}

int main(int argc, char** argv){
	argv_parse(opt, argc, argv);
	if( opt[OPT_h].set ){
		argv_usage(opt, argv[0]);
		return 0;
	}
	
	if( opt[OPT_builtin_lips_objdump].set ){
		__free uint16_t* gb = lipsByc_load_binary(opt[OPT_builtin_lips_objdump].value->str);
		if( !gb ) die("error on load file '%s': %m", opt[OPT_builtin_lips_objdump].value->str);
		lipsByc_s lbyc;
		if( !lipsByc_ctor(&lbyc, gb) ) return 1;
		lips_objdump(&lbyc, stdout);
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
		lips_match_ctor(&m);
		__free utf8_t* source = (utf8_t*)load_file(opt[OPT_g].value->str);
		lips_vm_reset(&vm, &m, source);
		if( lips_vm_match(&vm) < 1 ){
			lips_dump_error(&vm, &m, source, stderr);
			die("");
		}
		die("TODO extract new grammar");
		lipsByc_dtor(&lbyc);
		lips_vm_dtor(&vm);
		lips_match_dtor(&m);
		lipsByc_ctor(&lbyc, lgram);
		lips_vm_ctor(&vm, &lbyc);	
	}

	lipsMatch_s m;
	lips_match_ctor(&m);
	mforeach(opt[OPT_s].value, it){
		__free utf8_t* source = (utf8_t*)load_file(opt[OPT_s].value[it].str);
		lips_vm_reset(&vm, &m, source);

		if( opt[OPT_d].set ){
			lips_vm_match_debug(&vm);
		}
		else{
			lips_vm_match(&vm);
		}
		if( m.count < 1 ){
			lips_dump_error(&vm, &m, vm.txt, stderr);
			return 0;
		}
		if( opt[OPT_dump_capture].set ){
			FILE* out = argfopen(-1, opt[OPT_dump_capture].value[it].str, "w");
			lips_dump_capture(&m, out);
			argfclose(out);
		}
		
		if( m.ast.root ){
			lipsAst_s* root = m.ast.root;
			if( opt[OPT_dump_ast_split].set ){
				while( m_header(root->child)->len == 1 ) root = root->child[0];
				if( m_header(root->child)->len == 0 ) root = m.ast.root;
			}
			if( opt[OPT_dump_ast_file].set ){
				if( opt[OPT_dump_ast_split].set ){
					mforeach(root->child, i){
						FILE* out = argfopen(i, opt[OPT_dump_ast_file].value[it].str, "w");
						lips_dump_ast(&vm, root->child[i], out, 0);
						argfclose(out);
					}
				}
				else{
					FILE* out = argfopen(-1, opt[OPT_dump_ast_file].value[it].str, "w");
					lips_dump_ast(&vm, m.ast.root, out, 0);
					argfclose(out);
				}
			}
			if( opt[OPT_dump_ast_dot].set ){
				if( opt[OPT_dump_ast_split].set ){
					mforeach(root->child, i){
						FILE* out = argfopen(i, opt[OPT_dump_ast_dot].value[it].str, "w");
						lips_dump_ast(&vm, root->child[i], out, 1);
						argfclose(out);
						if( opt[OPT_dot_png].set ) dotbuild(i, opt[OPT_dump_ast_dot].value[it].str);
					}
				}
				else{
					FILE* out = argfopen(-1, opt[OPT_dump_ast_dot].value[it].str, "w");
					lips_dump_ast(&vm, m.ast.root, out, 1);
					argfclose(out);
					if( opt[OPT_dot_png].set ) dotbuild(-1, opt[OPT_dump_ast_dot].value[it].str);
				}
			}
			if( opt[OPT_dump_name_cenum].set ){
				FILE* out = argfopen(-1, opt[OPT_dump_name_cenum].value->str, "w");
				lips_dump_name_cenum(&lbyc, "grammarName", "LGRAM", out);
				argfclose(out);
			}
			if( opt[OPT_builtin_lips_emitter].set ){
				uint16_t* b = lips_builtin_emitter(&lbyc, m.ast.root);
				if( opt[OPT_emitter_ccode].set ) lipsByc_save_ccode(b, opt[OPT_emitter_ccode].value->str, opt[OPT_builtin_lips_emitter].value->str);
				else if( lipsByc_save_binary(b, opt[OPT_builtin_lips_emitter].value->str) < 0 ) die("error on save file '%s': %m", opt[OPT_builtin_lips_objdump].value->str);
			}
		}
	}
	
	lips_vm_dtor(&vm);
	lips_match_dtor(&m);
	lipsByc_dtor(&lbyc);
	return 0;
}

