#ifndef __MB_INUTILITY_H__
#define __MB_INUTILITY_H__

#include <notstd/core.h>

typedef struct termSurface{
	unsigned x;
	unsigned y;
	unsigned w;
	unsigned h;
}termSurface_s;

typedef struct termSurfaceLine{
	termSurface_s* surface;
	unsigned h;
}termSurfaceLine_s;

typedef struct termMultiSurface{
	unsigned x;
	unsigned y;
	unsigned w;
	termSurfaceLine_s* line;
}termMultiSurface_s;



const char* skip_h(const char* p, const char* end);
const char* skip_hn(const char* p, const char* end);
size_t count_line_to(const char* begin, const char* end);
const char* extract_line(const char* begin, const char* end, const char* from, const char** endLine);
char* dup_line(const char* begin, const char* end);
unsigned count_tab(const char* begin, const char* end);
void display_signal(size_t offset);
void error_show(unsigned errnum, const char* errstr, const char* errdesc, const char* begin, size_t len, size_t erroff);
char* load_file(const char* path);
const char* cast_view_char(unsigned ch, int convertnumerical);
void term_cls(void);
void term_gotoxy(unsigned x, unsigned y);
void term_nexty(unsigned y);
void term_prevx(unsigned x);
void term_fcol(unsigned col);
void term_bcol(unsigned col);
void term_reset(void);
void term_bold(void);
void term_box(unsigned i);
void term_hline(unsigned len);
void term_vline(unsigned len);
void term_cline(unsigned len);


#endif
