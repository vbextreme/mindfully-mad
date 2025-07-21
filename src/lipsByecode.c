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

lipsByc_s* lipsByc_ctor(lipsByc_s* byc, uint16_t* bytecode){
	dbg_info("");
	byc->format = bytecode[BYC_FORMAT];
	if( byc->format != BYTECODE_FORMAT ){
		dbg_error("wrong bytecode format");
		return NULL;
	}
	byc->bytecode      = bytecode;
	byc->flags         = bytecode[BYC_FLAGS];
	byc->rangeCount    = bytecode[BYC_RANGE_COUNT];
	byc->urangeCount   = bytecode[BYC_URANGE_COUNT];
	byc->fnCount       = bytecode[BYC_FN_COUNT];
	byc->errCount      = bytecode[BYC_ERR_COUNT];
	byc->start         = bytecode[BYC_START];
	byc->codeLen       = bytecode[BYC_CODELEN];
	byc->sectionFn     = bytecode[BYC_SECTION_FN];
	byc->sectionName   = bytecode[BYC_SECTION_NAME];
	byc->sectionError  = bytecode[BYC_SECTION_ERROR];
	byc->sectionRange  = bytecode[BYC_SECTION_RANGE];
	byc->sectionURange = bytecode[BYC_SECTION_URANGE];
	byc->sectionCode   = bytecode[BYC_SECTION_CODE];
	byc->fn            = &bytecode[byc->sectionFn];
	byc->range         = &bytecode[byc->sectionRange];
	byc->urange        = &bytecode[byc->sectionURange];
	byc->code          = &bytecode[byc->sectionCode];
	byc->fnName        = map_name(&bytecode[byc->sectionName], byc->fnCount);
	byc->errStr        = map_name(&bytecode[byc->sectionError], byc->errCount);
	return byc;
}

void lipsByc_dtor(lipsByc_s* byc){
	m_free(byc->fnName);
	m_free(byc->errStr);
}

long lipsByc_name_toid(lipsByc_s* byc, const char* name){
	mforeach(byc->fnName, i){
		if( !strcmp(byc->fnName[i], name) ) return i;
	}
	return -1;
}






