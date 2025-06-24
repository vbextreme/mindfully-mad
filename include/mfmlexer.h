#ifndef __MFM_LEXER_H__
#define __MFM_LEXER_H__

#include <notstd/core.h>
#include <notstd/regex.h>
#include <notstd/rbtree.h>

#define LEX_RULE_STORE        0x01
#define LEX_RULE_NOTBEGIN     0x02

#define LEX_FIRST_MATCH       0x01
#define LEX_BEST_MATCH        0x02
#define LEX_UNMATCH_SKIP      0x04
#define LEX_UNMATCH_ERROR     0x08

typedef struct lexRule{
	rbtNode_s node;
	char*     name;
	regex_s   rex;
	unsigned  flags;
}lexRule_s;

typedef struct lexToken{
	unsigned    ruleid;
	const char* begin;
	unsigned    len;
}lexToken_s;

typedef struct lex{
	lexRule_s*  rules;
	rbtree_s    trules;
	unsigned    flags;
	unsigned    cid;
}lex_s;


lex_s* lex_ctor(lex_s* lex);
lex_s* lex_dtor(lex_s* lex);
const char* lex_error_str(lex_s* lex);
int lex_rule_add(lex_s* lex, char* name, char* regex, unsigned flags, unsigned* offerr, const char** errstr);
int lex_tokenize(lex_s* lex, lexToken_s** reusableToken, char* txt, size_t len, unsigned* offerr);
lexRule_s* lex_find_rule_byname(lex_s* lex, const char* name);

#endif
