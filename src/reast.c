#include <mfmrevm.h>

__private reAst_s* recast(bcnode_s* node){
	unsigned j;
	reAst_s* start = NEW(reAst_s);
	start->sp = 0;
	start->id = REVM_NODE_START;
	start->parent = NULL;
	start->child = MANY(reAst_s, 2);
	reAst_s* current = start;
	dbg_info("created _start_");
	unsigned disable = 1;

	mforeach(node, i){
		if( !current ) die("internall error, list node corrupted");
		switch( node[i].op ){
			case NOP_NEW:
				if( disable ) break;
				j = m_ipush(&current->child);
				current->child[j].id = node[i].id;
				current->child[j].sp = node[i].sp;
				current->child[j].parent = current;
				current->child[j].child  = MANY(reAst_s, 2);
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



reAst_s* reast_make(bcnode_s* node){
	return recast(node);
}
