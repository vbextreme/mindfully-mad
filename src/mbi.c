#include <mfm.h>
#include <notstd/opt.h>
#include <unistd.h>
#include <fcntl.h>

typedef enum{
	OPT_g,
	OPT_s,
	OPT_dump_token,
	OPT_h
}options_e;

option_s opt[] = {
	{ 'g', "--grammar"   , "grammar file"         ,              OPT_PATH | OPT_EXISTS, 0, NULL},
	{ 's', "--source"    , "source file"          ,  OPT_ARRAY | OPT_PATH | OPT_EXISTS, 0, NULL},
	{ '0', "--dump-token", "dump generetion token",                          OPT_NOARG, 0, NULL},
	{ 'h', "--help"      , "display this"         ,                OPT_NOARG | OPT_END, 0, NULL},
};


int main(int argc, char** argv){
	argv_parse(opt, argc, argv);
	if( opt[OPT_h].set ){
		argv_usage(opt, argv[0]);
		return 0;
	}
	if( !opt[OPT_g].set )die("required argument --grammar");
	if( !opt[OPT_s].set )die("required argument --source");
	
	mfm_s mfm;
	mfm_s_ctor(&mfm);
	mfm_load_grammar(&mfm, opt[OPT_g].value->str);
	
	mforeach(opt[OPT_s].value, it){
		mfm_clear(&mfm);
		mfm_lexer_file(&mfm, opt[OPT_s].value[it].str);
		printf("<%s>\n", opt[OPT_s].value[it].str);
		if( opt[OPT_dump_token].set ) mfm_token_dump(&mfm);
	}
	mfm_s_dtor(&mfm);
}

