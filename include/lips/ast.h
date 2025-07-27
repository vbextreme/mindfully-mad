#ifndef __LIPS_AST_H__
#define __LIPS_AST_H__

#include <lips/bytecode.h>
#include <notstd/utf8.h>

typedef struct lipsAsl{
	lipsOP_e      op;
	uint16_t      id;
	const utf8_t* sp;
}lipsAsl_s;

typedef struct lipsAst{
	struct lipsAst* parent;
	struct lipsAst* child;
	const utf8_t*   tp;
	unsigned        len;
	uint16_t        id;
	const utf8_t*   orgtp;
	unsigned        orglen;
}lipsAst_s;

typedef struct lipsAstMatch{
	lipsAst_s*  root;
	lipsAst_s*  last;
	lipsAst_s** marked;
}lipsAstMatch_s;

lipsAstMatch_s lips_ast_make(lipsAsl_s* node);
void lips_ast_dtor(lipsAst_s* node);
void lips_ast_match_dtor(lipsAstMatch_s* m);
lipsAst_s* lips_ast_child_id(lipsAst_s* node, unsigned id);

/*
lipsAst_s* lips_ast_branch(lipsAst_s* node);
lipsAst_s* lips_ast_rev_root(lipsAst_s* last);
lipsAst_s** lips_ast_postorder(lipsAst_s* root);
lipsAst_s* lips_ast_child_id(lipsAst_s* node, unsigned id);
*/



#endif

