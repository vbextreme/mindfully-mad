#include <lips/bytecode.h>

__private const char** map_name(const uint16_t* section, const unsigned count){
	const char** ret = MANY(const char*, count);
	m_header(ret)->len = count;
	for( unsigned i = 0; i < count; ++i ){
		ret[i] = (const char*)section;
		unsigned len = strlen(ret[i])+1;
		len = ROUND_UP(len, 2);
		section += len/2;
	}
	return ret;
}

__private lipsSFase_s* map_semantic(const uint16_t* section, const unsigned count){
	lipsSFase_s* ret = MANY(lipsSFase_s, count);
	m_header(ret)->len = count;
	for( unsigned i = 0; i < count; ++i ){
		const unsigned cfase = *section++;
		ret[i].addr = MANY(uint16_t, cfase);
		m_header(ret[i].addr)->len = cfase;
		for( unsigned j = 0; j < cfase; ++j ){
			ret[i].addr[j] = *section++;
		}
	}
	return ret;
}

const char* lipsByc_extract_string(uint16_t* byc, uint32_t offset, uint32_t* next, unsigned* len){
	char* str = (char*)&byc[offset];
	*len = strlen(str);
	*next = ROUND_UP(*len, 2) / 2;
	return str;
}

lipsByc_s* lipsByc_ctor(lipsByc_s* byc, uint16_t* bytecode){
	dbg_info("");
	byc->format = bytecode[BYC_FORMAT];
	if( byc->format != BYTECODE_FORMAT ){
		dbg_error("wrong bytecode format");
		return NULL;
	}
	byc->bytecode        = bytecode;
	byc->flags           = bytecode[BYC_FLAGS];
	byc->rangeCount      = bytecode[BYC_RANGE_COUNT];
	byc->urangeCount     = bytecode[BYC_URANGE_COUNT];
	byc->fnCount         = bytecode[BYC_FN_COUNT];
	byc->nameCount       = bytecode[BYC_NAME_COUNT];
	byc->errCount        = bytecode[BYC_ERR_COUNT];
	byc->semanticCount   = bytecode[BYC_SEMANTIC_COUNT];
	byc->start           = bytecode[BYC_START];
	byc->codeLen         = bytecode[BYC_CODELEN];
	byc->sectionFn       = bytecode[BYC_SECTION_FN];
	byc->sectionName     = bytecode[BYC_SECTION_NAME];
	byc->sectionError    = bytecode[BYC_SECTION_ERROR];
	byc->sectionRange    = bytecode[BYC_SECTION_RANGE];
	byc->sectionURange   = bytecode[BYC_SECTION_URANGE];
	byc->sectionSemantic = bytecode[BYC_SECTION_SEMANTIC];
	byc->sectionCode     = bytecode[BYC_SECTION_CODE];
	byc->fn              = &bytecode[byc->sectionFn];
	byc->range           = &bytecode[byc->sectionRange];
	byc->urange          = &bytecode[byc->sectionURange];
	byc->code            = &bytecode[byc->sectionCode];
	byc->name            = map_name(&bytecode[byc->sectionName], byc->nameCount);
	byc->errStr          = map_name(&bytecode[byc->sectionError], byc->errCount);
	byc->sfase           = map_semantic(&bytecode[byc->sectionSemantic], byc->semanticCount);
	return byc;
}

void lipsByc_dtor(lipsByc_s* byc){
	m_free(byc->name);
	m_free(byc->errStr);
	mforeach(byc->sfase,i){
		m_free(byc->sfase[i].addr);
	}
	m_free(byc->sfase);
}

long lipsByc_name_toid(lipsByc_s* byc, const char* name){
	mforeach(byc->name, i){
		if( !strcmp(byc->name[i], name) ) return i;
	}
	return -1;
}






