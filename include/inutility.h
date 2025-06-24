#ifndef __MB_INUTILITY_H__
#define __MB_INUTILITY_H__

#include <notstd/core.h>

const char* skip_h(const char* p, const char* end);
const char* skip_hn(const char* p, const char* end);
size_t count_line_to(const char* begin, const char* end);
const char* extract_line(const char* begin, const char* end, const char* from, const char** endLine);
char* dup_line(const char* begin, const char* end);
unsigned count_tab(const char* begin, const char* end);
void display_signal(size_t offset);
void error_show(unsigned errnum, const char* errstr, const char* errdesc, const char* begin, size_t len, size_t erroff);
char* load_file(const char* path);
#endif
