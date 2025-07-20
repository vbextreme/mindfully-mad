#include <lips/ast.h>

lipsAst_s* lips_ast_make(lipsAsl_s* node){
	unsigned j;
	lipsAst_s* start = NEW(lipsAst_s);
	start->sp = 0;
	start->id = LIPS_NODE_START;
	start->parent = NULL;
	start->child = MANY(lipsAst_s, 2);
	lipsAst_s* current = start;
	unsigned disable = 0;

	mforeach(node, i){
		if( !current ) die("internall error, list node corrupted");
		switch( node[i].op ){
			case NOP_NEW:
				if( disable ) break;
				j = m_ipush(&current->child);
				current->child[j].id = node[i].id;
				current->child[j].sp = node[i].sp;
				current->child[j].parent = current;
				current->child[j].child  = MANY(lipsAst_s, 1);
				current = &current->child[j];
			break;
			case NOP_PARENT:
				if( disable ) break;
				current->len = node[i].sp - current->sp;
				current = current->parent;
			break;
			case NOP_ENABLE:
				iassert(disable);
				--disable;
			break;
			
			case NOP_DISABLE: ++disable; break;
		}
	}
	return start;
}

__private void recast_dtor(lipsAst_s* node){
	mforeach(node->child, i){
		recast_dtor(&node->child[i]);
	}
	m_free(node->child);
}

void lips_ast_dtor(lipsAst_s* node){
	if( !node ) return;
	recast_dtor(node);
	m_free(node);
}
