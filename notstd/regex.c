#include <notstd/regex.h>
#include <notstd/str.h>

#define PCRE2_CODE_UNIT_WIDTH 8
#include <pcre2.h>


//	pcre2_code* rx;
//	pcre2_match_data* match;
//	PCRE2_SIZE* offv;

regex_s* regex_ctor(regex_s* rx){
	rx->rx = NULL;
	rx->match = NULL;
	rx->offv = NULL;
	rx->offc = 0;
	rx->regex = NULL;
	rx->text = NULL;
	rx->lent = 0;
	rx->newline = 0;
	rx->utf8 = -1;
	rx->crlf = -1;
	rx->clearregex = 0;
	rx->rexflags = 0;
	rx->err  = 0;
	return rx;
}

regex_s* regex_text(regex_s* rx, const char* txt, size_t len){
	rx->text = txt;
	rx->lent = len ? len : strlen(txt);
	return rx;
}

regex_s* regex_match_delete(regex_s* rx){
	if( rx->match ){
		pcre2_match_data_free(rx->match);
		rx->match = NULL;
	}
	return rx;
}

regex_s* regex_dtor(regex_s* rx){
	regex_match_delete(rx);
	if( rx->rx ) pcre2_code_free(rx->rx);
	if( rx->clearregex ) m_free(rx->regex);
	return rx;
}

const char* regex_err_str(regex_s* rx){
	static __thread char buf[1024];
	buf[0] = 0;
	pcre2_get_error_message(rx->err, (uint8_t*)buf, sizeof(buf));
	return buf;
}

int regex_build(regex_s* rx, const char* regex){
	regex_dtor(rx);
	rx->regex = (char*)regex;
	rx->rx = pcre2_compile((PCRE2_SPTR)rx->regex, PCRE2_ZERO_TERMINATED, 0, &rx->err, &rx->erroff, NULL);
	if( !rx->rx ){
		errno = EINVAL;
		return -1;
	}
	return 0;
}

void regex_match_always_begin(regex_s* rx){
	rx->rexflags = PCRE2_ANCHORED;
}

__private int match_first(regex_s* rx){
	//dbg_info("MATCHSEARCH(%.5s)/%s/: %zu", rx->text, rx->regex, regex_match_count(rx));
	rx->match = pcre2_match_data_create_from_pattern(rx->rx, NULL);
	int rc = pcre2_match(rx->rx, (PCRE2_SPTR)rx->text, rx->lent, 0, rx->rexflags, rx->match, NULL);
	if (rc < 0){
		//dbg_warning("no match");
		switch(rc){
			case PCRE2_ERROR_NOMATCH: return 0;
			default:
				errno = ENOSTR;
				rx->err = rc;
			break;
		}
		return -1;
	}
	rx->offc = rc;
	rx->offv = pcre2_get_ovector_pointer(rx->match);
	if( rx->offv[0] > rx->offv[1] ){
		dbg_warning("off0 > off1");
		errno = EINVAL;
		return -1;
	}
	/*	
	dbg_info("ok match");
	unsigned i = 0;
	const char* m;
	unsigned long l;
	while( (m=regex_match_get(rx, i, &l)) ){
		dbg_info("  <%u>%.*s</%u>", i, (int)l, m, i);
		++i;
	}
	*/
	return 1;
}

__private void match_before_continue(regex_s* rx){
	uint32_t obits;
	pcre2_pattern_info(rx->rx, PCRE2_INFO_ALLOPTIONS, &obits);
	rx->utf8 = (obits & PCRE2_UTF) != 0;
	pcre2_pattern_info(rx->rx, PCRE2_INFO_NEWLINE, &rx->newline);
	rx->crlf = rx->newline == PCRE2_NEWLINE_ANY || rx->newline == PCRE2_NEWLINE_CRLF || rx->newline == PCRE2_NEWLINE_ANYCRLF;
}

__private int match_continue(regex_s* rx){
	if( rx->utf8 == 1 && rx->crlf == -1 ) match_before_continue(rx);
	
	for(;;){
		uint32_t options = 0;
		PCRE2_SIZE start_offset = rx->offv[1];
		
		if( rx->offv[0] == rx->offv[1]){
			if( rx->offv[0] == rx->lent) return -1;
			options = PCRE2_NOTEMPTY_ATSTART | PCRE2_ANCHORED;
		}
		else{
			PCRE2_SIZE startchar = pcre2_get_startchar(rx->match);
			if( start_offset <= startchar ){
				if (startchar >= rx->lent) return -1;
				start_offset = startchar + 1;
				if( rx->utf8){
				for (; start_offset < rx->lent; start_offset++)
					if ((rx->text[start_offset] & 0xc0) != 0x80) return -1;
				}
			}
		}
		
		int rc = pcre2_match(rx->rx, (PCRE2_SPTR)rx->text, rx->lent, start_offset, options, rx->match, NULL);
		if( rc == PCRE2_ERROR_NOMATCH){
			if (options == 0) return 0;
			rx->offv[1] = start_offset + 1;
			if( rx->crlf && start_offset < rx->lent - 1 && rx->text[start_offset] == '\r' && rx->text[start_offset + 1] == '\n'){
				rx->offv[1] += 1;
			}
			else if( rx->utf8 ){
				while( rx->offv[1] < rx->lent){
					if( (rx->text[rx->offv[1]] & 0xc0) != 0x80) return -1;
					rx->offv[1] += 1;
				}
			}
			continue;
		}
		if( rc < 0 ){
			rx->err = rc;
			errno = ENOSTR;
		    return -1;
		}
		rx->offc = rc;
		if( rx->offv[0] > rx->offv[1] ){
			errno = EINVAL;
			return -1;
		}
		break;
	}
	
	return 1;
}

int regex_match(regex_s* rx){
	if( !rx->rx ){
		errno = ENOSR;
		return -1;
	}
	if( rx->match ){
		return match_continue(rx);
	}
	else{
		return match_first(rx);
	}
	return 0;
}

size_t regex_match_count(regex_s* rx){
	return rx->offc;
}

const char* regex_match_get(regex_s* rx, unsigned index, unsigned* lenout){
	if( index >= rx->offc ){
		errno = EINVAL;
		return NULL;
	}
	const char* start = rx->text + rx->offv[2*index];
	if( lenout ) *lenout = rx->offv[2*index+1] - rx->offv[2*index];
	return start;
}

char** str_regex(const char* regex, const char* str, int global){
	regex_s rx;
	regex_ctor(&rx);
	if( regex_build(&rx, regex) ){
		return NULL;
	}
	regex_text(&rx, str, strlen(str));
	if( regex_match(&rx) ){
		regex_dtor(&rx);
		return NULL;
	}
	
	size_t count = regex_match_count(&rx);
	char** capture = MANY(char*, count);
	m_header(capture)->len = count;

	for(size_t i = 0; i < count; ++i){
		unsigned len = 0;
		const char* match = regex_match_get(&rx, i, &len);
		capture[i] = str_dup(match, len);
	}
	
	if( global ){
		while( !regex_match(&rx) ){
			count = regex_match_count(&rx);
			capture = m_grow(capture, count);
			m_header(capture)->len += count;
			for(size_t i = 0; i < count; ++i){
				unsigned len = 0;
				const char* match = regex_match_get(&rx, i, &len);
				capture[i] = str_dup(match, len);
			}
		}
	}
	
	regex_dtor(&rx);
	return capture;
}

