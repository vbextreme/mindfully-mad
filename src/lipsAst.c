#include <lips/ast.h>

lipsAstMatch_s lips_ast_make(lipsAsl_s* node){
	lipsAstMatch_s m;
	m.last   = NULL;
	m.marked = MANY(lipsAst_s*, 32);
	m.root    = NEW(lipsAst_s);
	m.root->tp     = 0;
	m.root->orgtp  = 0;
	m.root->len    = 0;
	m.root->orglen = 0;
	m.root->id     = LIPS_NODE_START;
	m.root->parent = NULL;
	m.root->child  = MANY(lipsAst_s, 2);
	lipsAst_s* current = m.root;
	unsigned j;
	unsigned n;
	unsigned disable = 0;
	mforeach(node, i){
		if( !current ) die("internall error, list node corrupted");
		switch( node[i].op ){
			default: die("internal error");
			case OPEV_NODEEX_NEW:
				if( disable ) break;
				j = m_ipush(&current->child);
				current->child[j].id     = node[i].id;
				current->child[j].tp     = node[i].sp;
				current->child[j].len    = 0;
				current->child[j].orgtp  = NULL;
				current->child[j].orglen = 0;
				current->child[j].parent = current;
				current->child[j].child  = MANY(lipsAst_s, 1);
				current = &current->child[j];
			break;
			case OPEV_NODEEX_PARENT:
				if( disable ) break;
				current->len = node[i].sp - current->tp;
				current = current->parent;
			break;
			case OPEV_NODEEX_ENABLE:
				iassert(disable);
				--disable;
			break;
			
			case OPEV_NODEEX_DISABLE: ++disable; break;
			
			case OPEV_NODEEX_MARK:
				n = m_ipush(&m.marked);
				m.marked[n] = current;
			break;
		}
	}
	m.last = current;
	return m;
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

void lips_ast_match_dtor(lipsAstMatch_s* m){
	lips_ast_dtor(m->root);
	m_free(m->marked);
}

lipsAst_s* lips_ast_child_id(lipsAst_s* node, unsigned id){
	mforeach(node->child, i){
		if( node->child[i].id == id ) return &node->child[i];
	}
	return NULL;
}

/*
lipsAst_s* lips_ast_branch(lipsAst_s* node){
	if( !node->parent ) return node;
	unsigned i;
	const unsigned n = m_header(node->parent)->len;
	for( i=0; i < n; ++i ){
		if( &node->parent->child[i] == node ) break;
	}
	iassert( i < n );
	lipsAst_s* root = NEW(lipsAst_s);
	memcpy(root, node, sizeof(lipsAst_s));
	m_delete(node->parent, i, 1);
	root->parent = NULL;
	return root;
}

lipsAst_s* lips_ast_rev_root(lipsAst_s* last){
	lipsAst_s* root = last->parent;
	while( root ){
		last = root;
		root = last->parent;
	}
	return last;
}


__private void po_ast(lipsAst_s*** po, lipsAst_s* node){
	mforeach(node->child, i){
		po_ast(po, &node->child[i]);
	}
	unsigned i = m_ipush(po);
	(*po)[i] = node;
}

lipsAst_s** lips_ast_postorder(lipsAst_s* root){
	lipsAst_s** po = MANY(lipsAst_s*, 512);
	po_ast(&po, root);
	return po;
}


*/





