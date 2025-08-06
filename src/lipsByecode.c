#include <lips/bytecode.h>
#include <unistd.h>
#include <fcntl.h>

__private const char** map_name(const uint16_t* section, const unsigned count){
	const char** ret = MANY(const char*, count);
	m_header(ret)->len = count;
	const char* n = (const char*) section;
	for( unsigned i = 0; i < count; ++i ){
		ret[i] = n;
		const unsigned len = strlen(ret[i])+1;
		n += ROUND_UP(len, 2);
		dbg_info("[%u] '%s'", i, ret[i]);
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
	*next = offset+ROUND_UP(*len+1, 2) / 2;
	return str;
}

lipsByc_s* lipsByc_ctor(lipsByc_s* byc, uint16_t* bytecode){
	dbg_info("");
	byc->format = bytecode[BYC_FORMAT];
	if( byc->format != BYTECODE_FORMAT ){
		dbg_error("wrong bytecode format, aspected %X but is %X", BYTECODE_FORMAT, bytecode[BYC_FORMAT]);
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
	
	dbg_info("flags:%X rangeCount:%u urange:%u fn:%u name:%u err:%u semantic:%u start:%u codelen:%u",
		byc->flags,
		byc->rangeCount,
		byc->urangeCount,
		byc->fnCount,
		byc->nameCount,
		byc->errCount,
		byc->semanticCount,
		byc->start,
		byc->codeLen
	);
	dbg_info("sections:: fn:%u name:%u error:%u range:%u urange:%u semantic:%u code:%u",
		byc->sectionFn,
		byc->sectionName,
		byc->sectionError,
		byc->sectionRange,
		byc->sectionURange,
		byc->sectionSemantic,
		byc->sectionCode
	);
	byc->fn              = &bytecode[byc->sectionFn];
	byc->range           = &bytecode[byc->sectionRange];
	byc->urange          = &bytecode[byc->sectionURange];
	byc->code            = &bytecode[byc->sectionCode];
	dbg_info("map name");
	byc->name            = map_name(&bytecode[byc->sectionName], byc->nameCount);
	dbg_info("map error");
	byc->errStr          = map_name(&bytecode[byc->sectionError], byc->errCount);
	dbg_info("map semantic");
	byc->sfase           = map_semantic(&bytecode[byc->sectionSemantic], byc->semanticCount);
	dbg_info("load completed");
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

int lipsByc_save_binary(uint16_t* bytecode, const char* fname){
	int fd = -1;
	if( !strcmp(fname, "stdout") ){
		fd = 1;
	}
	else if( !strcmp(fname, "stderr") ){
		fd = 2;
	}
	else{
		fd = open(fname, O_WRONLY | O_CREAT | O_TRUNC, 0600);
	}
	if( fd < 0 ) return -1;
	if( write(fd, bytecode, bytecode[BYC_SECTION_CODE]+bytecode[BYC_CODELEN] * sizeof(uint16_t)) < 0 ){
		close(fd);
		return -1;
	}
	close(fd);
	return 0;
}

uint16_t* lipsByc_load_binary(const char* path){
	dbg_info("load %s", path);
	int fd = open(path, 0, O_RDONLY);
	if( fd == -1 ) return NULL;
	uint16_t* buffer = MANY(uint16_t, 4096);
	ssize_t nr;
	while( (nr=read(fd, &buffer[m_header(buffer)->len], m_available(buffer)*2)) > 0 ){
		iassert(nr%2 == 0);
		m_header(buffer)->len += nr/2;
		buffer = m_grow(buffer, 4096);
	}
	if( nr < 0 ){
		m_free(buffer);
		return NULL;
	}
	close(fd);
	return buffer;
}

int lipsByc_save_ccode(uint16_t* bytecode, const char* varname, const char* fname){
	int fd = -1;
	if( !strcmp(fname, "stdout") ){
		fd = 1;
	}
	else if( !strcmp(fname, "stderr") ){
		fd = 2;
	}
	else{
		fd = open(fname, O_WRONLY | O_CREAT | O_TRUNC, 0600);
	}
	if( fd < 0 ) return -1;
	const unsigned bytecodelen = bytecode[BYC_CODELEN]+bytecode[BYC_SECTION_CODE];
	dprintf(fd, "uint16_t %s[] = {\n", varname);
	for( unsigned i = 0; i < bytecodelen; ++i ){
		dprintf(fd, "\t[0x%X] = 0x%X,\n", i, bytecode[i]);
	}
	dprintf(fd, "};\n");
	close(fd);
	return 0;
}






