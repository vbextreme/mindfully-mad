#include <lips/ccompiler.h>
#include <notstd/memory.h>
#include <notstd/str.h>
#include <limits.h>

lcc_s* lcc_ctor(lcc_s* rc){
	rc->bytecode = MANY(uint16_t, 64);
	rc->label    = MANY(lcclabel_s, 16);
	rc->fn       = MANY(lccfn_s, 16);
	rc->call     = MANY(lccfn_s, 16);
	rc->range    = MANY(lccrange_s, 16);
	rc->urange   = MANY(lccurange_s, 16);
	rc->errstr   = MANY(char*, 16);
	rc->err      = 0;
	lccrange_s any;
	lcc_range_ctor(&any);
	lcc_range_set(&any, 0);
	lcc_range_reverse(&any);
	lcc_range_add(rc, &any);
	lcc_label_new(rc);
	unsigned i = m_ipush(&rc->errstr);
	rc->errstr[i] = str_dup("ok",0);
	return rc;
}

void lcc_dtor(lcc_s* rc){
	m_free(rc->bytecode);
	m_free(rc->label);
	m_free(rc->range);
	m_free(rc->urange);
	mforeach(rc->fn, i) m_free(rc->fn[i].name);
	m_free(rc->fn);
	mforeach(rc->call, i) m_free(rc->call[i].name);
	m_free(rc->call);
	mforeach(rc->errstr, i) m_free(rc->errstr[i]);
	m_free(rc->errstr);
}

__private unsigned push_bytecode(lcc_s* lc, uint16_t byc){
	unsigned i = m_ipush(&lc->bytecode);
	lc->bytecode[i] = byc;
	return i;
}

lcc_s* lcc_match(lcc_s* rc){
	push_bytecode(rc, OP_MATCH);
	return rc;
}

lcc_s* lcc_char(lcc_s* rc, uint8_t val){
	push_bytecode(rc, OP_CHAR | val);
	return rc;
}

lcc_s* lcc_utf8(lcc_s* rc, const utf8_t* val){
	unsigned nb = utf8_bytes_count(val);
	for( unsigned i = 0; i < nb; ++i ){
		lcc_char(rc, *val++);
	}
	return rc;
}

lccrange_s* lcc_range_ctor(lccrange_s* range){
	memset(range->map, 0, sizeof(range->map));
	return range;
}

lccrange_s* lcc_range_set(lccrange_s* range, uint8_t ch){
	const unsigned im = ch & 0x0F;
	const unsigned bm = 1 << (ch>>4);
	range->map[im] |= bm;
	return range;
}

lccrange_s* lcc_range_clr(lccrange_s* range, uint8_t ch){
	const unsigned im = ch & 0x0F;
	const unsigned bm = 1 << (ch>>4);
	range->map[im] &= ~bm;
	return range;
}

lccrange_s* lcc_range_reverse(lccrange_s* range){
	lcc_range_set(range, 0);
	for( unsigned i = 0; i < 16; ++i ){
		range->map[i] = ~range->map[i];
	}
	return range;
}

lccrange_s* lcc_range_str_set(lccrange_s* range, const char* accept, unsigned rev){
	for( unsigned i = 0; accept[i]; ++i ){
		lcc_range_set(range, accept[i]);
	}
	if( rev ) lcc_range_reverse(range);
	return range;
}

long lcc_range_add(lcc_s* rc, lccrange_s* range){
	mforeach(rc->range, it){
		if( !memcmp(rc->range[it].map, range->map, sizeof range->map) ){
			return it;
		}
	}
	unsigned ret = m_ipush(&rc->range);
	memcpy(rc->range[ret].map, range->map, sizeof range->map);
	if( ret >= 4096 ){
		rc->err = LCC_ERR_RANGE_MANY;
		rc->errid   = ret;
		rc->erraddr = 0;
		return -1;
	}
	return ret;
}

lcc_s* lcc_range(lcc_s* rc, uint16_t val){
	push_bytecode(rc, OP_RANGE | (val&0x0FFF));
	return rc;
}

__private lcclabel_s* lbl_ctor(lcclabel_s* lbl){
	lbl->address = -1;
	lbl->resolve = MANY(unsigned, 1);
	return lbl;
}

unsigned lcc_label_new(lcc_s* rc){
	unsigned ret = m_ipush(&rc->label);
	lbl_ctor(&rc->label[ret]);
	dbg_info("label + %u", ret);
	return ret;
}

unsigned lcc_label(lcc_s* rc, unsigned lbl){
	rc->label[lbl].address = m_header(rc->bytecode)->len;
	dbg_info("label ! %u", lbl);
	return lbl;
}

__private void label_resolve(lcc_s* rc, unsigned lbl, unsigned bc){
	iassert(lbl < m_header(rc->label)->len);
	unsigned i = m_ipush(&rc->label[lbl].resolve);
	rc->label[lbl].resolve[i] = bc;
}

lcc_s* lcc_split(lcc_s* rc, unsigned lbl){
	label_resolve(rc, lbl,
		push_bytecode(rc, OP_SPLITF)
	);
	return rc;
}

lcc_s* lcc_splir(lcc_s* rc, unsigned lbl){
	label_resolve(rc, lbl,
		push_bytecode(rc, OP_SPLIRF)
	);
	return rc;
}

lcc_s* lcc_jmp(lcc_s* rc, unsigned lbl){
	label_resolve(rc, lbl,
		push_bytecode(rc, OP_JMPF)
	);
	return rc;
}

lcc_s* lcc_save(lcc_s* rc, uint8_t id){
	push_bytecode(rc, OP_EXT | OPE_SAVE | id);
	return rc;
}

lcc_s* lcc_node(lcc_s* rc, uint16_t id){
	push_bytecode(rc, OP_NODE | (id&0x0FFF));
	return rc;
}

lcc_s* lcc_nodeex(lcc_s* rc, nodeOP_e nop){
	push_bytecode(rc, OP_EXT | OPE_NODEEX | (nop&0xFF));
	return rc;
}

long lcc_fn(lcc_s* rc, const char* name, unsigned len){
	__free char* tmpname = str_dup(name, len);
	mforeach(rc->fn, i){
		if( !strcmp(rc->fn[i].name, tmpname) ){
			rc->err   = LCC_ERR_FN_REDEFINITION;
			rc->errid = i;
			return -1;
		}
	}
	unsigned ret = m_ipush(&rc->fn);
	if( ret > 4096 ){
		rc->err     = LCC_ERR_FN_MANY;
		rc->errid   = ret;
		return -1;
	}
	dbg_info("fn[%u] '%s'", ret, tmpname);
	unsigned addr =  m_header(rc->bytecode)->len;
	if( addr > UINT16_MAX ){
		rc->err     = LCC_ERR_FN_LONG;
		rc->errid   = ret;
		rc->erraddr = addr;
		return -1;
	}
	rc->fn[ret].addr = addr;
	rc->fn[ret].name = m_borrowed(tmpname);
	rc->fn[ret].fail = lcc_label_new(rc);
	lcc_split(rc, rc->fn[ret].fail);
	rc->currentFN = ret;
	return ret;
}

lcc_s* lcc_ret(lcc_s* rc){
	push_bytecode(rc, OP_EXT | OPE_RET);
	if( rc->label[rc->fn[rc->currentFN].fail].address == -1 ){
		lcc_label(rc, rc->fn[rc->currentFN].fail);
		push_bytecode(rc, OP_EXT | OPE_RET | 0x00FF);
	}
	dbg_info("ret %d", rc->currentFN);
	return rc;
}

int lcc_fn_prolog(lcc_s* rc, const char* name, unsigned len, unsigned store){
	lcc_fn(rc, name, len);
	if( store ) lcc_node(rc, rc->currentFN);
	return 1;
}

int lcc_fn_epilog(lcc_s* rc, unsigned stored){
	if( stored ) lcc_nodeex(rc, NOP_PARENT);
	lcc_ret(rc);
	return 0;
}

int lcc_calli(lcc_s* rc, unsigned ifn){
	if( ifn >= m_header(rc->fn)->len ){
		rc->err     = LCC_ERR_FN_NOTEXISTS;
		rc->errid   = ifn;
		rc->erraddr = -1;
		return -1;
	}
	push_bytecode(rc, OP_CALL | (ifn & 0x0FFF));
	return 0;
}

lcc_s* lcc_call(lcc_s* rc, const char* name, unsigned len){
	unsigned bc = m_ipush(&rc->bytecode);
	rc->bytecode[bc] = OP_CALL;
	unsigned res = m_ipush(&rc->call);
	rc->call[res].addr = bc;
	rc->call[res].name = str_dup(name, len);
	return rc;
}

long lcc_error_add(lcc_s* rc, const char* str, unsigned len){
	unsigned i = m_ipush(&rc->errstr);
	if( i >= 256 ){
		rc->err = LCC_ERR_ERR_MANY;
		rc->errid   = i;
		rc->erraddr = 0;
		return -1;
	}
	rc->errstr[i] = str_dup(str, len);
	return i;
}

lcc_s* lcc_error(lcc_s* rc, uint8_t num){
	push_bytecode(rc, OP_EXT | OPE_ERROR | num);
	return rc;
}

lcc_s* lcc_start(lcc_s* rc, int search){
	if( search ){
		//.*?
		unsigned next   = lcc_label_new(rc);
		lcc_label(rc, 0);
		lcc_splir(rc, next);
		lcc_range(rc, 0);
		lcc_jmp(rc, 0);
		lcc_label(rc, next);
	}
	else{
		lcc_label(rc, 0);
	}
	lcc_save(rc, 0);
	return rc;
}

__private long name_to_ifn(lcc_s* rc, const char* name){
	mforeach(rc->fn, i){
		if( !strcmp(rc->fn[i].name, name) ) return i;
	}
	return -1;
}

uint16_t* lcc_make(lcc_s* rc){
	if( rc->err ) return NULL;
	
	unsigned totalheader = BYC_HEADER_SIZE +
		m_header(rc->range)->len * 16 +
		m_header(rc->fn)->len
	;
	mforeach(rc->fn, i){
		unsigned len = strlen(rc->fn[i].name);
		len = ROUND_UP(len, 2);
		totalheader += len;
	}
	mforeach(rc->errstr, i){
		unsigned len = strlen(rc->errstr[i]);
		len = ROUND_UP(len, 2);
		totalheader += len;
	}
	
	if( rc->label[0].address >= 65536 ){
		rc->err     = LCC_ERR_LBL_LONG;
		rc->errid   = 0;
		rc->erraddr = rc->label[0].address;
		return NULL;
	}
	
	unsigned totalbc = totalheader + m_header(rc->bytecode)->len;
	uint16_t* bc = MANY(uint16_t, totalbc);
	bc[BYC_FORMAT]       = BYTECODE_FORMAT;
	bc[BYC_FLAGS]        = 0;
	bc[BYC_RANGE_COUNT]  = m_header(rc->range)->len;
	bc[BYC_URANGE_COUNT] = m_header(rc->urange)->len;
	bc[BYC_FN_COUNT]     = m_header(rc->fn)->len;
	bc[BYC_ERR_COUNT]    = m_header(rc->errstr)->len;
	bc[BYC_START]        = rc->label[0].address;
	bc[BYC_CODELEN]      = m_header(rc->bytecode)->len;
	uint16_t* inc = &bc[BYC_HEADER_SIZE];
	//.section fn
	bc[BYC_SECTION_FN] = inc - bc;
	mforeach(rc->fn, i){
		if( rc->fn[i].addr > UINT16_MAX ){
			rc->err     = LCC_ERR_FN_LONG;
			rc->errid   = i;
			rc->erraddr = rc->fn[i].addr;
			m_free(bc);
			return NULL;
		}
		*inc++ = rc->fn[i].addr;
	}
	//.section name
	bc[BYC_SECTION_NAME] = inc - bc;
	mforeach(rc->fn, i){
		unsigned len = strlen(rc->fn[i].name)+1;
		memcpy(inc, rc->fn[i].name, len);
		len = ROUND_UP(len, 2);
		len /= 2;
		inc += len;
	}
	//.section err
	bc[BYC_SECTION_ERROR] = inc - bc;
	mforeach(rc->errstr, i){
		unsigned len = strlen(rc->errstr[i])+1;
		memcpy(inc, rc->errstr[i], len);
		len = ROUND_UP(len, 2);
		len /= 2;
		inc += len;
	}	
	//.section range
	bc[BYC_SECTION_RANGE] = inc - bc;
	mforeach(rc->range, i){
		memcpy(inc, rc->range[i].map, sizeof rc->range[i].map);
		inc += (sizeof rc->range[i].map)/2;
	}
	//.section urange
	bc[BYC_SECTION_URANGE] = inc - bc;
	//linker
	mforeach(rc->label, it){
		if( rc->label[it].address == -1 ){
			rc->err     = LCC_ERR_LBL_UNDECLARED;
			rc->errid   = it;
			rc->erraddr = -1;
			m_free(bc);
			return NULL;
		}
		mforeach(rc->label[it].resolve, i){
			const unsigned ba  = rc->label[it].resolve[i];
			uint16_t byc = rc->bytecode[ba];
			uint16_t off;
			unsigned op = BYTECODE_CMD40(byc);
			if( rc->label[it].address < ba ){
				off = ba - rc->label[it].address;
				op += 0x1000;
			}
			else{
				off = rc->label[it].address - ba;
			}

			if( off > 4096 ){
				rc->err     = LCC_ERR_JMP_LONG;
				rc->errid   = it;
				rc->erraddr = off;
				m_free(bc);
				return NULL;
			}
			rc->bytecode[ba] = op | off;
		}
	}
	mforeach(rc->call, it){
		long ifn = name_to_ifn(rc, rc->call[it].name);
		if( ifn == -1 ){
			rc->err     = LCC_ERR_FN_UNDECLARED;
			rc->errid   = it;
			rc->erraddr = 0;
			m_free(bc);
			return NULL;
		}
		rc->bytecode[rc->call[it].addr] |= ifn & 0x0FFF;
	}
	//.section code
	bc[BYC_SECTION_CODE] = inc - bc;
	memcpy(inc, rc->bytecode, m_header(rc->bytecode)->len*sizeof(uint16_t));

	return bc;
}

const char* lcc_err_str(lcc_s* lc, char info[4096]){
	switch( lc->err ){
		default                     : sprintf(info, "%u is not internal error", lc->err); break;
		case LCC_ERR_OK             : sprintf(info, "ok"); break;
		case LCC_ERR_ERR_MANY       : sprintf(info, "declared too many error, > 256"); break;
		case LCC_ERR_RANGE_MANY     : sprintf(info, "declared too many range, > 4096"); break;
		case LCC_ERR_JMP_LONG       : sprintf(info, "label %lu long jump, address 0x%lX > 4096", lc->errid, lc->erraddr); break;
		case LCC_ERR_LBL_UNDECLARED : sprintf(info, "label %lu is not declared", lc->errid); break;
		case LCC_ERR_LBL_LONG       : sprintf(info, "label start jump over %lu", lc->erraddr); break;
		case LCC_ERR_FN_REDEFINITION: sprintf(info, "redefinition fn '%s'", lc->fn[lc->errid].name); break;
		case LCC_ERR_FN_LONG        : sprintf(info, "fn %s long jump, address 0x%lX > 65536", lc->fn[lc->errid].name, lc->erraddr); break;
		case LCC_ERR_FN_MANY        : sprintf(info, "declared too many fn, > 4096"); break;
		case LCC_ERR_FN_NOTEXISTS   : sprintf(info, "fn %lu not exists", lc->errid); break;
		case LCC_ERR_FN_UNDECLARED  : sprintf(info, "undeclared fn '%s'", lc->fn[lc->errid].name); break;
	}
	return info;
}

void lcc_err_die(lcc_s* lc){
	if( !lc->err ) return;
	char tmp[4096];
	
	die("lcc error(%u): %s", 
		lc->err,
		lcc_err_str(lc, tmp)
	);
}



























