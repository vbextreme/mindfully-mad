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
	OPT_h
}options_e;

option_s opt[] = {
	{ 'g', "--grammar"   , "grammar file"             ,              OPT_PATH | OPT_EXISTS, 0, NULL},
	{'\0', "--reasm-gram", "use builtin reasm grammar",                          OPT_NOARG, 0, NULL},
	{ 's', "--source"    , "source file"              ,  OPT_ARRAY | OPT_PATH | OPT_EXISTS, 0, NULL},
	{ 'd', "--debug"     , "debug"                    ,                          OPT_NOARG, 0, NULL},
	{ 'h', "--help"      , "display this"             ,                OPT_NOARG | OPT_END, 0, NULL},
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
	
	mforeach(opt[OPT_s].value, it){
	//	mfm_clear(&mfm);
	//	printf("<%s>\n", opt[OPT_s].value[it].str);
	//	mfm_lexer_file(&mfm, opt[OPT_s].value[it].str);
	//	if( opt[OPT_dump_token].set ) mfm_token_dump(&mfm);
		if( opt[OPT_d].set ){
			__free utf8_t* source = (utf8_t*)load_file(opt[OPT_s].value[it].str);
			revm_debug(grambyc, source);
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
