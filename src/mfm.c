#include <mfm.h>
#include <inutility.h>
#include <notstd/json.h>

mfm_s* mfm_s_ctor(mfm_s* mfm){
	lex_ctor(&mfm->lexer);
	mfm->token  = NULL;
	mfm->source = NULL;
	mfm->fname  = NULL;
	return mfm;
}

mfm_s* mfm_s_dtor(mfm_s* mfm){
	lex_dtor(&mfm->lexer);
	m_free(mfm->token);
	m_free(mfm->source);
	return mfm;
}

mfm_s* mfm_clear(mfm_s* mfm){
	if( mfm->token ) m_clear(mfm->token);
	mfm->fname = NULL;
	m_free(mfm->source);
	mfm->source = NULL;
	return mfm;
}

__private void mfm_die(const char* action, int errnum, const char* errstr, const char* errdesc, const char* src, size_t offerr){
	fprintf(stderr, "mindfully-mad %s error(%d): %s", action, errnum, errstr);
	if( errdesc ) fprintf(stderr, "(%s)", errdesc);
	fputc('\n', stderr);
	const char* endLine;
	const char* beginLine    = extract_line(src, src+strlen(src), src+offerr, &endLine);
	const unsigned offsetOut = fprintf(stderr, "%3zu | ", count_line_to(src, src+offerr));
	unsigned ct              = count_tab(beginLine, endLine);
	__free char* line = dup_line(beginLine, endLine);
	fprintf(stderr, "%s\n", line);
	unsigned sig = offerr - (beginLine-src);
	sig = (sig-ct);
	sig += offsetOut;
	while( ct-->0 ){
		fputc(' ', stderr);
		fputc(' ', stderr);
	}
	display_signal(sig);
	exit(1);
}

__private jvalue_s* jv_property_ordie(jvalue_s* jv, jvtype_e type, const char* name, const char* src){
	jvalue_s* ret = jvalue_property(jv, name);
	if( ret->type == JV_ERR ) mfm_die("load grammar", -1, "missing property", name, src, jv->link - src);
	if( ret->type != type   ) mfm_die("load grammar", -1, "wrong property type, aspected", jvalue_type_to_name(type), src, ret->link - src);
	return ret;
}

__private void load_grammar_json(mfm_s* mfm, const char* src){
	const char* end;
	const char* outerr;
	__free jvalue_s* jg = json_decode(src, &end, &outerr);
	if( !jg ) mfm_die("json-parse", -1, outerr, NULL, src, end-src);
	if( jg->type != JV_OBJECT ) mfm_die("load grammar", -1, "grammar aspected json object", NULL, src, jg->link - src);
	
	jvalue_s* jlexer = jv_property_ordie(jg, JV_OBJECT, "lexer", src);

	jvalue_s* jgeneric = jv_property_ordie(jlexer, JV_STRING, "matchMode", src);
	if( !strcmp(jgeneric->s, "first") ){
		mfm->lexer.flags |= LEX_FIRST_MATCH;
	}
	else if( !strcmp(jgeneric->s, "best") ){
		mfm->lexer.flags |= LEX_BEST_MATCH;
	}
	else{
		mfm_die("load grammar", -1, "unknown matchMode", "first/best", src, jgeneric->link - src);
	}
	
	jgeneric = jv_property_ordie(jlexer, JV_STRING, "unmatchMode", src);
	if( !strcmp(jgeneric->s, "skip") ){
		mfm->lexer.flags |= LEX_UNMATCH_SKIP;
	}
	else if( !strcmp(jgeneric->s, "error") ){
		mfm->lexer.flags |= LEX_UNMATCH_ERROR;
	}
	else{
		mfm_die("load grammar", -1, "unknown unmatchMode", "skip/error", src, jgeneric->link - src);
	}
	
	jvalue_s* jrule = jv_property_ordie(jlexer, JV_ARRAY, "rule", src);
	mforeach(jrule->a, i){
		if( jrule->a[i].type != JV_OBJECT ) mfm_die("load grammar", -1, "wrong property type, aspected", jvalue_type_to_name(JV_OBJECT), src, jrule->a[i].link - src);
		jvalue_s* jname  = jv_property_ordie(&jrule->a[i], JV_STRING, "name", src);
		jvalue_s* jregex = jv_property_ordie(&jrule->a[i], JV_STRING, "regex", src);
		jvalue_s* jstore = jvalue_property(&jrule->a[i], "store");
		jvalue_s* jnobeg = jvalue_property(&jrule->a[i], "notbegin");
		unsigned flags = 0;
		switch( jstore->type ){
			case JV_ERR    : flags |= LEX_RULE_STORE; break;
			case JV_BOOLEAN: if( jstore->b) flags |= LEX_RULE_STORE; break;
			default        :  mfm_die("load grammar", -1, "wrong property type, aspected", jvalue_type_to_name(JV_BOOLEAN), src, jstore->link - src);
		}
		switch( jnobeg->type ){
			case JV_ERR    : break;
			case JV_BOOLEAN: if( jnobeg->b) flags |= LEX_RULE_NOTBEGIN; break;
			default        : mfm_die("load grammar", -1, "wrong property type, aspected", jvalue_type_to_name(JV_BOOLEAN), src, jnobeg->link - src);
		}	
		unsigned offerr;
		const char* errstr;
		if( lex_rule_add(&mfm->lexer, m_borrowed(jname->s), m_borrowed(jregex->s), flags, &offerr, &errstr) ) {
			mfm_die("load grammar", -1, "build regex", errstr, src, (jregex->link-src)+offerr);
		}
	}
}

void mfm_load_grammar(mfm_s* mfm, const char* fname){
	__free char* src = load_file(fname);
	load_grammar_json(mfm, src);
}

void mfm_lexer(mfm_s* mfm, const char* fname, char* src){
	unsigned offerr;
	mfm->source = src;
	mfm->fname  = fname;
	if( lex_tokenize(&mfm->lexer, &mfm->token, src, strlen(src), &offerr) ) mfm_die("lexer", -1, "unknown rule for generate token on file", fname, src, offerr);
}

void mfm_lexer_file(mfm_s* mfm, const char* fname){
	mfm_lexer(mfm, fname, load_file(fname));
}

void mfm_token_dump(mfm_s* mfm){
	mforeach(mfm->token, i){
		fputs(mfm->lexer.rules[mfm->token[i].ruleid].name, stdout);
		const char* dsp = mfm->token[i].begin;
		unsigned n = mfm->token[i].len;
		unsigned nl = 0;
		putchar('(');
		while( n-->0 ){
			if( *dsp == '\n' ){
				fputs("\\n", stdout);
				nl=1;
			}
			else{
				putchar(*dsp);
			}
			++dsp;
		}
		putchar(')');
		if( nl ) putchar('\n');
	}
	puts("");
}


















