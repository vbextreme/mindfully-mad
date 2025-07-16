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

const char* cast_view_char(unsigned ch, int convertnumerical){
	static char tmp[64];
	tmp[0] = 0;
	tmp[1] = 0;
	if( ch == '\n' ){
		return "↵";
	}
	else if( ch == '\t' ){
		return "→";
	}
	else if( ch == ' '  ){
		return "␣";
	}
	else if( ch < 32 ){
		if( convertnumerical ) sprintf(tmp, "%3d", ch);
	}
	else if( ch < 128 ) {
		tmp[0] = ch;
	}
	else if( convertnumerical ){
		sprintf(tmp, "%3d", ch);
	}
	return tmp;
}

void term_cls(void){
	fputs("\033[2J\033[H", stdout);
}

void term_gotoxy(unsigned x, unsigned y){
	printf("\033[%d;%dH", y+1, x+1);
}

void term_nexty(unsigned y){
	printf("\033[%dB", y);
}

void term_prevx(unsigned x){
	printf("\033[%dD", x);
}

void term_fcol(unsigned col){
	printf("\033[38;5;%um", col);
}

void term_bcol(unsigned col){
	printf("\033[48;5;%um", col);
}

void term_reset(void){
	printf("\033[m");
}

void term_bold(void){
	printf("\033[1m");
}

void term_box(unsigned i){
	//                            0    1    2    3    4    5    6    7    8
	static const char* box[] = { "┌", "┬", "┐", "┤", "┘", "┴", "└", "├", "┼"};
	fputs(box[i], stdout);
}

void term_hline(unsigned len){
	while( len --> 0 ) fputs("─", stdout);
}

void term_vline(unsigned len){
	while( len --> 0 ){
		fputs("│", stdout);
		term_nexty(1);
		term_prevx(1);
	}
}

void term_cline(unsigned len){
	while( len --> 0 ) putchar(' ');
}

termMultiSurface_s* term_multi_surface_ctor(termMultiSurface_s* m, unsigned x, unsigned y,  unsigned sw){
	m->line = MANY(termSurfaceLine_s, 1);
	m->w = sw;
	m->x = x;
	m->y = y;
	return m;
}

termMultiSurface_s* term_multi_surface_vsplit(termMultiSurface_s* m, unsigned h){
	const unsigned i = m_ipush(&m->line);
	m->line[i].h = h;
	m->line[i].surface = MANY(termSurface_s, 1);
	return m;
}

termMultiSurface_s* term_multi_surface_hsplit(termMultiSurface_s* m, unsigned w){
	const unsigned c = m_header(m->line)->count - 1;
	const unsigned i = m_ipush(&m->line[c].surface);
	m->line[c].surface[i].w = w;
	return m;
}

__private unsigned tms_wsize(termSurface_s* s, unsigned* nu){
	unsigned w = 0;
	*nu = 0;
	mforeach(s, i){
		if( s->w ){
			++(*nu);
			w += s->w;
		}
	}
	return w;
}

termMultiSurface_s* term_multi_surface_apply(termMultiSurface_s* m){
	unsigned x = m->x;
	unsigned y = m->y;
	mforeach(m->line, il){
		termSurfaceLine_s* sl = &m->line[il];
		unsigned       nu;
		const unsigned wc = m_header(sl)->len;
		const unsigned dw = tms_wsize(sl->surface, &nu);
		const unsigned ws = (m->w - dw) / (wc-nu);
		mforeach(sl->surface, is){
			sl->surface[is].h = sl->h;
			if( !sl->surface[is].w ) sl->surface[is].w = ws;
			sl->surface[is].y = y;
			sl->surface[is].x = x;
			x += sl->surface[is].w-1;
		}
		y += sl->h -1;
	}
	return m;
}

termMultiSurface_s* term_multi_surface_draw(termMultiSurface_s* m){
	unsigned x = m->x;
	unsigned y = m->y;
	const unsigned lc = m_header(m->line)->len;
	for(unsigned il = 0; il < lc; ++il){
		termSurfaceLine_s* sl = &m->line[il];
		const unsigned sc = m_header(sl->surface)->len;
		for(unsigned is = 0; is < sc; ++is){
			
			x += sl->surface[is].w-1;
		}
		y += sl->h -1;
	}
	return m;
}












