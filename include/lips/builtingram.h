#ifndef __LIPS_BUILTIN_GRAM_H__
#define __LIPS_BUILTIN_GRAM_H__

#include <stdint.h>
#include <lips/bytecode.h>
#include <lips/ast.h>

uint16_t* lips_builtin_grammar_make(void);
uint16_t* lips_builtin_emitter(lipsByc_s* byc, lipsAst_s* ast);

#endif
