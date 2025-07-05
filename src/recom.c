#include <notstd/utf8.h>
#include <notstd/memory.h>
#include <mfmrecom.h>
#include <notstd/str.h>
#include <limits.h>

recom_s* recom_ctor(recom_s* rc){
	rc->bytecode = MANY(uint16_t, 64);
	rc->label    = MANY(reclabel_s, 16);
	rc->fn       = MANY(recfn_s, 16);
	rc->call     = MANY(recfn_s, 16);
	rc->range    = MANY(recrange_s, 16);
	rc->urange   = MANY(recurange_s, 16);
	
	recrange_s any;
	recom_range_ctor(&any);
	recom_range_set(&any, 0);
	recom_range_reverse(&any);
	recom_range_add(rc, &any);
	recom_label_new(rc);
	return rc;
}

void recom_dtor(recom_s* rc){
	m_free(rc->bytecode);
	m_free(rc->label);
	m_free(rc->range);
	m_free(rc->urange);
	mforeach(rc->fn, i) m_free(rc->fn[i].name);
	m_free(rc->fn);
	mforeach(rc->call, i) m_free(rc->call[i].name);
	m_free(rc->call);
}

recom_s* recom_match(recom_s* rc){
	unsigned i = m_ipush(&rc->bytecode);
	rc->bytecode[i] = OP_MATCH;
	return rc;
}

recom_s* recom_char(recom_s* rc, uint8_t val){
	unsigned i = m_ipush(&rc->bytecode);
	rc->bytecode[i] = OP_CHAR | val;
	return rc;
}

recom_s* recom_utf8(recom_s* rc, const utf8_t* val){
	unsigned nb = utf8_bytes_count(val);
	for( unsigned i = 0; i < nb; ++i ){
		recom_char(rc, *val++);
	}
	return rc;
}

recrange_s* recom_range_ctor(recrange_s* range){
	memset(range->map, 0, sizeof(range->map));
	return range;
}

recrange_s* recom_range_set(recrange_s* range, uint8_t ch){
	const unsigned im = ch & 0x0F;
	const unsigned bm = 1 << (ch>>4);
	range->map[im] |= bm;
	return range;
}

recrange_s* recom_range_clr(recrange_s* range, uint8_t ch){
	const unsigned im = ch & 0x0F;
	const unsigned bm = 1 << (ch>>4);
	range->map[im] &= ~bm;
	return range;
}

recrange_s* recom_range_reverse(recrange_s* range){
	for( unsigned i = 0; i < 16; ++i ){
		range->map[i] = ~range->map[i];
	}
	return range;
}

unsigned recom_range_add(recom_s* rec, recrange_s* range){
	mforeach(rec->range, it){
		if( !memcmp(rec->range[it].map, range->map, sizeof range->map) ){
			return it;
		}
	}
	unsigned ret = m_ipush(&rec->range);
	memcpy(rec->range[ret].map, range->map, sizeof range->map);
	if( ret >= 4096 ) die("to many range, max 4096");
	return ret;
}

recom_s* recom_range(recom_s* rc, uint16_t val){
	unsigned i = m_ipush(&rc->bytecode);
	rc->bytecode[i] = OP_RANGE | (val&0x0FFF);
	return rc;
}

__private reclabel_s* lbl_ctor(reclabel_s* lbl){
	lbl->address = -1;
	lbl->resolve = MANY(unsigned, 1);
	return lbl;
}

unsigned recom_label_new(recom_s* rc){
	unsigned ret = m_ipush(&rc->label);
	lbl_ctor(&rc->label[ret]);
	return ret;
}

unsigned recom_label(recom_s* rc, unsigned lbl){
	rc->label[lbl].address = m_header(rc->bytecode)->len;
	return lbl;
}

__private void label_resolve(recom_s* rc, unsigned lbl, unsigned bc){
	unsigned i = m_ipush(&rc->label[lbl].resolve);
	rc->label[lbl].resolve[i] = bc;
}

recom_s* recom_split(recom_s* rc, unsigned lbl){
	unsigned bc = m_ipush(&rc->bytecode);
	rc->bytecode[bc] = OP_SPLITF;
	label_resolve(rc, lbl, bc);
	return rc;
}

recom_s* recom_splir(recom_s* rc, unsigned lbl){
	unsigned bc = m_ipush(&rc->bytecode);
	rc->bytecode[bc] = OP_SPLIRF;
	label_resolve(rc, lbl, bc);
	return rc;
}

recom_s* recom_jmp(recom_s* rc, unsigned lbl){
	unsigned bc = m_ipush(&rc->bytecode);
	rc->bytecode[bc] = OP_JMPF;
	label_resolve(rc, lbl, bc);
	return rc;
}

recom_s* recom_save(recom_s* rc, uint8_t id){
	unsigned i = m_ipush(&rc->bytecode);
	rc->bytecode[i] = OP_EXT | OPE_SAVE | id;
	return rc;
}

recom_s* recom_node(recom_s* rc, uint16_t id){
	unsigned i = m_ipush(&rc->bytecode);
	rc->bytecode[i] = OP_NODE | (id&0x0FFF);
	return rc;
}

unsigned recom_fn(recom_s* rc, const char* name, unsigned len){
	unsigned ret = m_ipush(&rc->fn);
	unsigned addr =  m_header(rc->bytecode)->len;
	if( addr > UINT16_MAX ) die("function address to long %u", addr);
	rc->fn[ret].addr = addr;
	rc->fn[ret].name = str_dup(name, len);
	return ret;
}

recom_s* recom_calli(recom_s* rc, unsigned ifn){
	unsigned bc = m_ipush(&rc->bytecode);
	if( ifn > 4096 ) die("to many function");
	rc->bytecode[bc] = OP_CALL | (ifn & 0x0FFF);
	return rc;
}

recom_s* recom_call(recom_s* rc, const char* name, unsigned len){
	unsigned bc = m_ipush(&rc->bytecode);
	rc->bytecode[bc] = OP_CALL;
	unsigned res = m_ipush(&rc->call);
	rc->call[res].addr = bc;
	rc->call[res].name = str_dup(name, len);
	return rc;
}

recom_s* recom_ret(recom_s* rc){
	unsigned i = m_ipush(&rc->bytecode);
	rc->bytecode[i] = OP_EXT | OPE_RET;
	return rc;
}

recom_s* recom_parent(recom_s* rc){
	rc->bytecode[m_ipush(&rc->bytecode)] = OP_EXT | OPE_PARENT;
	return rc;
}

recom_s* recom_start(recom_s* rc, int search){
	if( search ){
		//.*?
		unsigned next   = recom_label_new(rc);
		recom_label(rc, 0);
		recom_splir(rc, next);
		recom_range(rc, 0);
		recom_jmp(rc, 0);
		recom_label(rc, next);
	}
	else{
		recom_label(rc, 0);
	}
	recom_save(rc, 0);
	return rc;
}

__private long name_to_ifn(recom_s* rc, const char* name){
	mforeach(rc->fn, i){
		if( !strcmp(rc->fn[i].name, name) ) return i;
	}
	return -1;
}

uint16_t* recom_make(recom_s* rc){
	unsigned totalheader = BYC_HEADER_SIZE +
		m_header(rc->range)->len * 16 +
		m_header(rc->fn)->len
	;
	mforeach(rc->fn, i){
		unsigned len = strlen(rc->fn[i].name);
		len = ROUND_UP(len, 2);
		totalheader += len;
	}
	
	unsigned totalbc = totalheader + m_header(rc->bytecode)->len;
	uint16_t* bc = MANY(uint16_t, totalbc);
	bc[BYC_FORMAT]       = BYTECODE_FORMAT;
	bc[BYC_FLAGS]        = 0;
	bc[BYC_RANGE_COUNT]  = m_header(rc->range)->len;
	bc[BYC_URANGE_COUNT] = m_header(rc->urange)->len;
	bc[BYC_FN_COUNT]     = m_header(rc->fn)->len;
	bc[BYC_START]        = rc->label[0].address;
	bc[BYC_CODELEN]      = m_header(rc->bytecode)->len;
	
	//.section fn
	uint16_t* inc = &bc[BYC_HEADER_SIZE];
	bc[BYC_SECTION_FN] = inc - bc;
	mforeach(rc->fn, i){
		if( rc->fn[i].addr > UINT16_MAX ) die("linker error: function call outside address %u", rc->fn[i].addr);
		*inc++ = rc->fn[i].addr;
	}
	//.section range
	bc[BYC_SECTION_RANGE] = inc - bc;
	mforeach(rc->range, i){
		memcpy(inc, rc->range[i].map, sizeof(uint16_t)*16);
		inc += 16;
	}
	//.section urange
	bc[BYC_SECTION_URANGE] = inc - bc;
	//.section name
	bc[BYC_SECTION_NAME] = inc - bc;
	mforeach(rc->fn, i){
		unsigned len = strlen(rc->fn[i].name)+1;
		memcpy(inc, rc->fn[i].name, len);
		len = ROUND_UP(len, 2);
		len /= 2;
		inc += len;
	}
	
	//linker
	mforeach(rc->label, it){
		if( rc->label[it].address == -1 ) die("linker error: unknow label address");
		mforeach(rc->label[it].resolve, i){
			const unsigned bc  = rc->label[it].resolve[i];
			uint16_t byc = rc->bytecode[bc];
			uint16_t off;
			unsigned op = BYTECODE_CMD40(byc);
			if( rc->label[it].address < bc ){
				off = bc - rc->label[it].address;
				op += 0x1000;
			}
			else{
				off = rc->label[it].address - bc;
			}
			if( off > 4096 ) die("linker error: jump to long %u", off);
			rc->bytecode[bc] = op | off;
		}
	}
	mforeach(rc->call, it){
		long ifn = name_to_ifn(rc, rc->call[it].name);
		if( ifn == -1 ) die("linker error: unable CALL, unknown function name '%s'", rc->call[it].name);
		rc->bytecode[rc->call[it].addr] |= ifn & 0x0FFF;
	}
	//.section code
	bc[BYC_SECTION_CODE] = inc - bc;
	memcpy(inc, rc->bytecode, m_header(rc->bytecode)->len*sizeof(uint16_t));
	return bc;
}


