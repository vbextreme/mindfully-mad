#include "notstd/memory.h"
#include "notstd/regex.h"
#include <notstd/str.h>

#include <mfmlexer.h>
#include <inutility.h>

__private int rule_name_cmp(const void* ra, const void* rb){
	return strcmp(((lexRule_s*)ra)->name, ((lexRule_s*)rb)->name);
}

lex_s* lex_ctor(lex_s* lex){
	dbg_info("");
	lex->rules = MANY(lexRule_s, 16);
	rbtree_ctor(&lex->trules, rule_name_cmp);
	lex->cid   = 0;
	lex->flags = 0;
	return lex;
}

lex_s* lex_dtor(lex_s* lex){
	mforeach(lex->rules, it){
		regex_dtor(&lex->rules[it].rex);
		m_free(lex->rules[it].name);
	}
	m_free(lex->rules);
	return lex;
}

int lex_rule_add(lex_s* lex, char* name, char* regex, unsigned flags, unsigned* offerr, const char** errstr){
	dbg_info("add %c rule %s: /%s/", flags & LEX_RULE_STORE ? '+' : '-', name, regex);
	unsigned i = m_ipush(&lex->rules);
	regex_ctor(&lex->rules[i].rex);
	if( regex_build(&lex->rules[i].rex, regex) ){
		*offerr = lex->rules[i].rex.erroff;
		*errstr = regex_err_str(&lex->rules[i].rex);
		m_ipop(lex->rules);
		return -1;
	}
	regex_match_always_begin(&lex->rules[i].rex);
	lex->rules[i].rex.clearregex = 1;
	lex->rules[i].name = name;
	lex->rules[i].flags = flags;
	rbtNode_ctor(&lex->rules[i].node, &lex->rules[i]);
	rbtree_insert(&lex->trules, &lex->rules[i].node);
	return 0;
}

__private const char* rule_match(lexRule_s* rule, const char* txt, const char* end, int beginLine, unsigned* len){
	if( (rule->flags & LEX_RULE_NOTBEGIN) && beginLine ){
		*len = 0;
		return NULL;
	}
	regex_text(&rule->rex, txt, end-txt);
	regex_match_delete(&rule->rex);
	if( regex_match(&rule->rex) > 0 && regex_match_count(&rule->rex) > 0){
		//dbg_info("rule: %s match", rule->name);
		return regex_match_get(&rule->rex, 0, len);
	}
	*len = 0;
	return NULL;
}

__private long rules_first_match(lex_s* lex, const char* txt, const char* end, int beginLine, const char** begin, unsigned* len){
	mforeach(lex->rules, it){
		*begin = rule_match(&lex->rules[it], txt, end, beginLine, len);
		if( *begin && *len ){
			//dbg_info("match(%.*s)", *len, *begin);
			return it;
		}
	}
	return -1;
}

__private long rules_best_match(lex_s* lex, const char* txt, const char* end, int beginLine, const char** begin, unsigned* len){
	long ret = -1;
	*len   = 0;
	*begin = NULL;
	mforeach(lex->rules, it){
		unsigned l;
		const char* m = rule_match(&lex->rules[it], txt, end, beginLine, &l);
		if( m && l ){
			if( l > *len ){
				*len   = l;
				*begin = m;
				ret = it;
			}
		}
	}
	return ret;
}

__private const char* txt_increase(const char* current, int* beginLine, unsigned count){
	while( count-->0 ){
		if( *beginLine ){
			switch( *current ){
				case ' ': case '\t': case '\n': break;
				default : *beginLine = 0;
			}
		}
		else if( *current == '\n' ){
			*beginLine = 1;
		}
		++current;
	}
	return current;
}

int lex_tokenize(lex_s* lex, lexToken_s** reusableToken, char* fsrc, size_t len, unsigned* offerr){
	if( !len ) len = strlen(fsrc);
	const char* txt = fsrc;
	const char* end = txt+len;
	lexToken_s* token = *reusableToken;
	if( !token ){
		token = MANY(lexToken_s, 32);
	}
	else{
		m_clear(token);
	}
	int beginLine = 1;

	while( txt < end ){
		const char* beginToken;
		unsigned    lenToken;
		long idr = lex->flags & LEX_FIRST_MATCH?
			rules_first_match(lex, txt, end, beginLine, &beginToken, &lenToken):
			rules_best_match(lex, txt, end, beginLine, &beginToken, &lenToken)
		;
		if( idr == -1 ){
			if( lex->flags & LEX_UNMATCH_ERROR ){
				*offerr = txt - fsrc;
				*reusableToken = token;
				return -1;
			}
			lenToken = 1;
		}
		else if( lex->rules[idr].flags & LEX_RULE_STORE ){
			unsigned i = m_ipush(&token);
			token[i].begin = beginToken;
			token[i].len   = lenToken;
			token[i].ruleid= idr;
			//dbg_info("store:%.*s len:%u idr:%lu", lenToken, beginToken, lenToken, idr);
		}
		txt = txt_increase(txt, &beginLine, lenToken);
	}
	*reusableToken = token;
	return 0;
}

lexRule_s* lex_find_rule_byname(lex_s* lex, const char* name){
	rbtNode_s* node = rbtree_find(&lex->trules, name);
	return node ? node->data : NULL;
}










