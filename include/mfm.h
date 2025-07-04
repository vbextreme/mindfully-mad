#ifndef __MFM_H__
#define __MFM_H__

#include <notstd/core.h>
#include <mfmlexer.h>

typedef struct mfm{
	lex_s       lexer;
	lexToken_s* token;
	char*       source;
	const char* fname;
}mfm_s;

mfm_s* mfm_s_ctor(mfm_s* mfm);
mfm_s* mfm_s_dtor(mfm_s* mfm);
mfm_s* mfm_clear(mfm_s* mfm);
void mfm_load_grammar(mfm_s* mfm, const char* fname);
void mfm_lexer(mfm_s* mfm, const char* fname, char* src);
void mfm_lexer_file(mfm_s* mfm, const char* fname);
void mfm_token_dump(mfm_s* mfm);

uint16_t* regram_make(void);

#endif
