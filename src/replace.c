#include <mbreplace.h>
#include <inutility.h>
#include <limits.h>
#include <notstd/str.h>

//HO SCAZZATO NON DEVO PASSARE TOKENS MA LEXPARSER è LI CHE CI SONO I NOMI DELLE REGOLE E DI CONSEGUENZA DA QUELLI ESTRAGGO

__private const char* extract_token(mbrep_s* rep, lexToken_s* tokens, lexToken_s** out, const char* p, const char* e){
	const char* tkstart = p;
	p = grammar_token_get(p, e);
	if( !p ){
		rep->err    = REP_ERR_INVALID_TOKEN_NAME;
		rep->offerr = tkstart - rep->txt;
		return NULL;
	}
	*out = lextoken_find_byname(tokens, tkstart, p - tkstart);
	if( !*out ){
		rep->err    = REP_ERR_LEX_TOKEN_NOT_EXISTS;
		rep->offerr = tkstart - rep->txt;
		return NULL;
	}
	return p;
}


__private const char* extract_reptoken(mbrep_s* rep, lexToken_s* tokens, repToken_s* out, const char* p, const char* e){
	p = extract_token(rep, tokens, &out->token, p, e);
	if( !p ) return NULL;
	switch( *p ){
		case '*':
			out->min = 0;
			out->max = UINT_MAX;
			++p;
		break;
		case '?':
			out->min = 0;
			out->max = 1;
			++p;
		break;
		case '+':
			out->min = 1;
			out->max = UINT_MAX;
			++p;
		break;
		default:
			out->min = 1;
			out->max = 1;
		break;
	}
	return p;
}

__private const char* extract_rule(mbrep_s* rep, lexToken_s* tokens, repRule_s* out, const char* p, const char* e){
	if( *p == '^' ){
		out->start = 1;
		++p;
	}
	do{
		unsigned ip = mem_ipush(&out->search);
		if( !(p = extract_reptoken(rep, tokens, &out->search[ip], p, e)) ) return NULL;
		p=skip_h(p, e);
	}while( p<e && !(p[0] == '-' && p[1] == '>') && *p != '\n' );
	if( p >= e || *p == '\n' ){
		rep->err    = REP_ERR_ASPECTED_REPLACER;
		rep->offerr = p - rep->txt;
	}
	p = skip_h(p+2,e);
	while( p<e && *p != '\n'){
		unsigned ip = mem_ipush(&out->replace);
		p = extract_token(rep, tokens, &out->replace[ip], p, e);
		if( !p ) return NULL;
		p = skip_h(p,e);
	}
	return *p == '\n' ? skip_hn(p,e): NULL;
}

const char* mbrep_rules_from_string(mbrep_s* rep, lexToken_s* tokens, const char* txt, size_t len, const char* from){
	if( !len ) len = strlen(txt);
	dbg_info("");
	const char* p = from;
	const char* e = txt + len;
	rep->txt    = txt;
	rep->txtLen = len;
	while( p < e ){
		p = skip_hn(p, e);
		if( *p == '!' ){
			if( strncmp(p, "!END", 4) ){
				rep->err    = REP_ERR_COSTRUCTOR_ASPECTED_END;
				rep->offerr = p-txt;
				return NULL;
			}
			p = skip_h(p+4, e);
			if( *p != '\n' ){
				rep->err    = REP_ERR_COSTRUCTOR_ASPECTED_END;
				rep->offerr = p-txt;
				return NULL;
			}
			return p+1;
		}
		else if( *p == '#' ){
			while( p < e && *p != '\n' ) ++p;
			continue;
		}
		unsigned nr = mem_ipush(&rep->rule);
		rep->rule[nr].replace = MANY(lexToken_s*, 1);
		rep->rule[nr].search  = MANY(repToken_s, 1);
		rep->rule[nr].start   = 0;
		if( !(p = extract_rule(rep, tokens, &rep->rule[nr], p, e)) ) return NULL;
	}
	return p;
}

int mbrep_load_rules(mbrep_s* lex, lexToken_s* tokens, const char* txt){
	dbg_info("");
	const char* begin = grammar_find_mark(txt, "!REPLACE");
	if( !begin ) return 0;
	if( !mbrep_rules_from_string(lex, tokens, txt, strlen(txt), begin) ) return -1;
	return 0;
}

mbrep_s* mbrep_ctor(mbrep_s* rep){
	rep->err = 0;
	rep->rule = MANY(repRule_s, 1);
	return rep;
}

mbrep_s* mbrep_dtor(mbrep_s* rep){
	mforeach(rep->rule, it){
		mem_free(rep->rule[it].replace);
		mem_free(rep->rule[it].search);
	}
	mem_free(rep->rule);
	return rep;
}

__private int test_rule(repRule_s* rule, lexToken_s* token, size_t* it, int start){
	if( rule->start && !start ) return -1;
	unsigned const count = mem_header(rule->search)->len;
	unsigned ir = 0;
	while( ir < count ){
		if( strcmp(rule->search[ir].token->name, token[*it].name) ){
			if( rule->search[ir].min == 0 ){
				++ir;
				continue;
			}
			return -1;
		}
		++(*it);
		if( rule->search[ir].max > 1 && ir < count - 1 ) {
			size_t next = *it;
			for( unsigned i = 1; i < rule->search[ir].max; ++i ){
				if( strcmp(rule->search[ir].token->name, token[next].name) ){
					break;
				}
				else{
					++next;
				}
			}
			*it = next;
		}
		++ir;
	}
	return 0;
}

__private lexToken_s* replace_rule(lexToken_s* tokens, lexToken_s** replace, size_t itStart, size_t* it){
	tokens = mem_delete(tokens, itStart, *it - itStart);
	const unsigned add = mem_header(replace)->len;
	if( !add ) return tokens;
	tokens = mem_widen(tokens, itStart, add);
	for( unsigned i = 0; i < add; ++i, ++itStart ){
		tokens[itStart] = *replace[i];
	}
}

__private lexToken_s* match_rule(mbrep_s* rep, lexToken_s* tokens, size_t* it, int start){
	size_t startRep = *it;
	mforeach(rep->rule, ir){
		if( !test_rule(&rep->rule[ir], tokens, it, start) ){
			//TODO REPLACE it not point to next but before becase at end of loop ++it
			return tokens;
		}
		else{
			*it = startRep;
		}
	}
	++it;
	return tokens;
}

lexToken_s* mbrep_run(mbrep_s* rep, lexToken_s* tokens, lexToken_s* nl){
	size_t it = 0;
	const size_t count = mem_header(tokens)->len;
	int start = 1;
	while( it < count ){
		tokens = match_rule(rep, tokens, &it, start);
		start = !!strcmp(tokens[it].name, nl->name);
	}
	return tokens;
}





