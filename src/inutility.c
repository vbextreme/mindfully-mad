#include <inutility.h>
#include <notstd/str.h>
#include <unistd.h>
#include <fcntl.h>

const char* skip_h(const char* p, const char* end){
	while( p < end && *p != '\n' && (*p == ' ' || *p == '\t') ) ++p;
	return p;
}

const char* skip_hn(const char* p, const char* end){
	while( p < end && (*p == ' ' || *p == '\t' || *p == '\n') ) ++p;
	return p;
}

size_t count_line_to(const char* begin, const char* end){
	size_t nr = 1;
	while( begin < end ){
		if( *begin == '\n' ) ++nr;
		++begin;
	}
	return nr;
}

const char* extract_line(const char* begin, const char* end, const char* from, const char** endLine){
	const char* f = from;
	while( f > begin && *f != '\n' ) --f;
	if( *f == '\n' ) ++f;
	begin = f;
	f = from;
	while( f < end && *f != '\n' ) ++f;
	if( endLine ) *endLine = f;
	return begin;
}

char* dup_line(const char* begin, const char* end){
	char* str = MANY(char, (end-begin)*2+1);
	char* ret = str;
	while( begin < end ){
		if( *begin == '\t' ){
			*str++ = ' ';
			*str++ = ' ';
		}
		else{
			*str++ = *begin;
		}
		++begin;
	}
	*str = 0;
	return ret;
}

unsigned count_tab(const char* begin, const char* end){
	unsigned count = 0;
	while( begin < end ){
		if( *begin == '\t' ) ++count;
		++begin;
	}
	return count;
}

void display_signal(size_t offset){
	while( offset-->0 ) fputc(' ', stderr);
	fputc('^', stderr);
	fputc('\n', stderr);
}

void error_show(unsigned errnum, const char* errstr, const char* errdesc, const char* begin, size_t len, size_t erroff){
	fprintf(stderr, "error(%d): %s", errnum, errstr);
	if( errdesc ) fprintf(stderr, "(%s)", errdesc);
	fputc('\n', stderr);
	const char* endLine;
	const char* beginLine    = extract_line(begin, begin+len, begin+erroff, &endLine);
	const unsigned offsetOut = fprintf(stderr, "%3zu | ", count_line_to(begin, begin+erroff));
	fprintf(stderr, "%.*s\n", (unsigned)(endLine - beginLine), beginLine);
	display_signal(offsetOut + erroff - (beginLine - begin));
}

char* load_file(const char* path){
	dbg_info("load %s", path);
	int fd = open(path, 0, O_RDONLY);
	if( fd == -1 ) die("error on open file %s: %m", path);
	char* buffer = MANY(char, 4096*2);
	ssize_t nr;
	while( (nr=read(fd, &buffer[m_header(buffer)->len], m_available(buffer))) > 0 ){
		m_header(buffer)->len += nr;
		buffer = m_grow(buffer, 4096*2);
	}
	if( nr < 0 ) die("error on read file %s: %m", path);
	close(fd);
	buffer = m_nullterm(buffer);
	return buffer;
}

