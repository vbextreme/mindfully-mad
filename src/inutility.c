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
	//                            0    1    2    3    4    5    6    7    8    9   10
	static const char* box[] = { "┌", "┬", "┐", "┤", "┘", "┴", "└", "├", "┼", "│", "─"};
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
	dbg_info("%u %u %u", x, y, sw);
	m->line = MANY(termSurfaceLine_s, 1);
	m->w = sw;
	m->x = x;
	m->y = y;
	return m;
}

termMultiSurface_s* term_multi_surface_vsplit(termMultiSurface_s* m, unsigned h){
	dbg_info("h:%u", h);
	const unsigned i = m_ipush(&m->line);
	m->line[i].h = h;
	m->line[i].surface = MANY(termSurface_s, 1);
	return m;
}

termMultiSurface_s* term_multi_surface_hsplit(termMultiSurface_s* m, unsigned w){
	const unsigned c = m_header(m->line)->len - 1;
	dbg_info("w:%u", w);
	const unsigned i = m_ipush(&m->line[c].surface);
	m->line[c].surface[i].w = w;
	return m;
}

__private unsigned tms_wsize(termSurface_s* s){
	unsigned w = 0;
	mforeach(s, i){
		w += s[i].w;
	}
	dbg_info("%u", w);
	return w;
}

__private unsigned tms_wmax(termMultiSurface_s* m){
	unsigned max = 0;
	mforeach(m->line, il){
		const unsigned w = tms_wsize(m->line[il].surface);
		if( w > max ) max = w;
	}
	dbg_info("%u", max);
	return max;
}

__private unsigned tms_hsize(termSurfaceLine_s* l){
	unsigned h = 0;
	mforeach(l, i){
		h += l[i].h;
	}
	dbg_info("%u", h);
	return h;
}

__private void mapset_clr(int8_t* map, unsigned maxw, unsigned x, unsigned y, unsigned w, unsigned h){
	dbg_info("max:%u x:%u y:%u w:%u h:%u", maxw,x,y,w,h);
	unsigned l = y*maxw;
	for( unsigned i = x; i < x+w; ++i ) map[l+i] = 0;
	l = (y+h-1)*maxw;
	for( unsigned i = x; i < x+w; ++i ) map[l+i] = 0;
	for( unsigned i = y; i < y+h; ++i ){
		map[i*maxw+x]     = 0;
		map[i*maxw+x+w-1] = 0;
	}
}

#define M_U 0x01
#define M_R 0x02
#define M_D 0x04
#define M_L 0x08

__private unsigned mapmoore(int8_t* map, unsigned x, unsigned y, unsigned w){
	static int8_t castmap[] = {
		[0]   = 8,
		[M_U] = 9,
		[M_D] = 9,
		[M_L] = 10,
		[M_R] = 10,
		[M_U | M_D] = 9,
		[M_L | M_R] = 10,
		[M_D | M_R] = 0,
		[M_D | M_L | M_R] = 1,
		[M_D | M_L] = 2,
		[M_U | M_D | M_L ] = 3,
		[M_U | M_L] = 4,
		[M_U | M_L | M_R] = 5,
		[M_U | M_R] = 6,
		[M_U | M_D | M_R] = 7,
		[M_U | M_D | M_L | M_R] = 8,
	};
	unsigned ms = 0;
	if( map[(y-1)*w+x] != -1 ) ms |= M_U;
	if( map[(y+1)*w+x] != -1 ) ms |= M_D;
	if( map[y*w+x-1] != -1 ) ms |= M_L;
	if( map[y*w+x+1] != -1 ) ms |= M_R;
	return castmap[ms];
}

__private void mapset_apply(int8_t* map, unsigned w, unsigned h){
	dbg_info("");
	for( unsigned iy = 1; iy < h-1; ++iy ){
		const unsigned l = iy * w;
		for( unsigned ix = 1; ix < w-1; ++ix ){
			if( map[l+ix] == -1 ) continue;
			map[l+ix] = mapmoore(map, ix, iy, w);
		}
	}
}

termMultiSurface_s* term_multi_surface_apply(termMultiSurface_s* m){
	dbg_info("");
	unsigned x = m->x;
	unsigned y = m->y;
	m->wmax    = tms_wmax(m)+2;
	m->hmax    = tms_hsize(m->line)+2;
	dbg_info("map %u*%u = %u", m->wmax, m->hmax, m->wmax * m->hmax);
	m->map     = MANY(int8_t, m->wmax*m->hmax);
	m_header(m->map)->len = m->wmax * m->hmax;
	for( unsigned i = 0; i < m->wmax * m->hmax; ++i ){
		m->map[i] = -1;
	}
	mforeach(m->line, il){
		termSurfaceLine_s* sl = &m->line[il];
		mforeach(sl->surface, is){
			sl->surface[is].h = sl->h;
			sl->surface[is].y = y;
			sl->surface[is].x = x;
			mapset_clr(m->map, m->wmax, (x-m->x)+1, (y-m->y)+1, sl->surface[is].w, sl->h);
			x += sl->surface[is].w-1;
		}
		x = 0;
		y += sl->h-1;
	}
	mapset_apply(m->map, m->wmax, m->hmax);
	//dump_map(m);
	return m;
}

termMultiSurface_s* term_multi_surface_draw(termMultiSurface_s* m){
	unsigned x = m->x;
	unsigned y = m->y;
	for(unsigned iy = 1; iy < m->hmax-2; ++iy){
		unsigned l = iy * m->wmax;
		term_gotoxy(x,y);
		for(unsigned ix = 1; ix < m->wmax-1; ++ix){
			const int8_t ch = m->map[l+ix];
			if( ch == -1 ) putchar(' ');
			else term_box(ch);
		}
		++y;
	}
	return m;
}

void term_surface_clear(termSurface_s* s){
	for( unsigned y = s->y + 1; y < s->h-2; ++y ){
		term_gotoxy(s->x+1, y);
		term_cline(s->w-2);
	}
}

void term_surface_focus(termSurface_s* s){
	term_gotoxy(s->x+1, s->y+1);
}

void term_flush(void){
	fflush(stdout);
}






