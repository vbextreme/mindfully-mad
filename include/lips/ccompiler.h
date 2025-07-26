#ifndef __LIPS_C_COMPILER_H__
#define __LIPS_C_COMPILER_H__

#include <lips/bytecode.h>
#include <lips/ast.h>
#include <notstd/utf8.h>

typedef struct lcclabel{
	long      address;
	unsigned* resolve;
}lcclabel_s;

typedef struct lccrange{
	uint16_t map[16];
}lccrange_s;

typedef struct lccurange{
	uint16_t map[16];
	uint16_t uni[512];
}lccurange_s;

typedef struct lccfn{
	unsigned addr;
	char*    name;
	unsigned fail;
}lccfn_s;

typedef struct lccSFase{
	uint16_t* addr;
}lccSFase_s;

typedef enum {
	LCC_ERR_OK,
	LCC_ERR_ERR_MANY,
	LCC_ERR_RANGE_MANY,
	LCC_ERR_JMP_LONG,
	LCC_ERR_LBL_UNDECLARED,
	LCC_ERR_LBL_LONG,
	LCC_ERR_FN_REDEFINITION,
	LCC_ERR_FN_LONG,
	LCC_ERR_FN_MANY,
	LCC_ERR_FN_NOTEXISTS,
	LCC_ERR_FN_UNDECLARED,
	LCC_ERR_ERR_OVERFLOW,
	LCC_ERR_UNKNOWN_NODE
}lccerr_e;

typedef struct lcc{
	uint16_t*    bytecode;
	lccfn_s*     fn;
	char**       name;
	lccfn_s*     call;
	lcclabel_s*  label;
	lccrange_s*  range;
	lccurange_s* urange;
	lccSFase_s*  sfase;
	char**       errstr;
	unsigned     currentFN;
	lccerr_e     err;
	long         errid;
	long         erraddr;
	const char*  erstr;
}lcc_s;


lcc_s* lcc_ctor(lcc_s* rc);
void lcc_dtor(lcc_s* rc);
unsigned lcc_push_name(lcc_s* rc, const char* name, unsigned len);
lcc_s* lcc_match(lcc_s* rc);
lcc_s* lcc_char(lcc_s* rc, uint8_t val);
lcc_s* lcc_utf8(lcc_s* rc, const utf8_t* val);
lccrange_s* lcc_range_ctor(lccrange_s* range);
lccrange_s* lcc_range_set(lccrange_s* range, uint8_t ch);
lccrange_s* lcc_range_clr(lccrange_s* range, uint8_t ch);
lccrange_s* lcc_range_reverse(lccrange_s* range);
lccrange_s* lcc_range_str_set(lccrange_s* range, const char* accept, unsigned rev);
long lcc_range_add(lcc_s* rec, lccrange_s* range);
lcc_s* lcc_range(lcc_s* rc, uint16_t val);
unsigned lcc_label_new(lcc_s* rc);
unsigned lcc_label(lcc_s* rc, unsigned lbl);
lcc_s* lcc_split(lcc_s* rc, unsigned lbl);
lcc_s* lcc_splir(lcc_s* rc, unsigned lbl);
lcc_s* lcc_jmp(lcc_s* rc, unsigned lbl);
lcc_s* lcc_save(lcc_s* rc, uint8_t id);
lcc_s* lcc_node(lcc_s* rc, uint16_t id);
lcc_s* lcc_nodeex(lcc_s* rc, lipsOP_e nop);
long lcc_fn(lcc_s* rc, const char* name, unsigned len);
lcc_s* lcc_ret(lcc_s* rc);
int lcc_fn_prolog(lcc_s* rc, const char* name, unsigned len, unsigned store, unsigned error);
int lcc_fn_epilog(lcc_s* rc, unsigned stored);
int lcc_calli(lcc_s* rc, unsigned ifn);
lcc_s* lcc_call(lcc_s* rc, const char* name, unsigned len);
long lcc_error_add(lcc_s* rc, const char* str, unsigned len);
lcc_s* lcc_error(lcc_s* rc, uint8_t num);
lcc_s* lcc_start(lcc_s* rc, int search);
lcc_s* lcc_semantic_fase_new(lcc_s* rc);
lcc_s* lcc_semantic_rule_new(lcc_s* rc);
lcc_s* lcc_enter(lcc_s* rc, const char* name, unsigned len);
uint16_t* lcc_make(lcc_s* rc);
lcc_s* lcc_type(lcc_s* rc, const char* name, unsigned len);
lcc_s* lcc_leave(lcc_s* rc);
lcc_s* lcc_value(lcc_s* rc, unsigned settest, const char* str, unsigned len);
lcc_s* lcc_symbol(lcc_s* rc, const char* name, unsigned len);
lcc_s* lcc_scope(lcc_s* rc, unsigned nls);
const char* lcc_err_str(lcc_s* lc, char info[4096]);
void lcc_err_die(lcc_s* lc);

#define CTOR()      lcc_s _lccobj; lcc_ctor(&_lccobj)
#define INIT(R)     lcc_s* _lcc = (R)
#define ROBJ()      (&_lccobj)
#define DTOR()      do{ lcc_dtor(_lcc); }while(0)
#define MATCH()     do{ lcc_match(_lcc); }while(0)
#define CHAR(CH)    do{ lcc_char(_lcc, CH); }while(0)
#define UNI(UTF8)   do{ lcc_utf8(_lcc, UTF8); }while(0)
#define USERANGE(R) lccrange_s _tmprange; lcc_range_ctor(&_tmprange)
#define RRANGE()    do{ lcc_range_ctor(&_tmprange); }while(0)
#define RRSET(CH)   do{ lcc_range_set(&_tmprange, CH); }while(0)
#define RRREV(CH)   do{ lcc_range_reverse(&_tmprange); }while(0)
#define RRSTR(S,R)  do{ lcc_range_str_set(&_tmprange, S, R);}while(0)
#define RRADD()     ({ long r = lcc_range_add(_lcc, &_tmprange); if( r < 0 ) lcc_err_die(_lcc); r; })
#define RANGE(ID)   do{ lcc_range(_lcc, ID); }while(0)
#define ANY()       RANGE(0)
#define USENELBL(NAME,N) unsigned NAME[N]; for( unsigned i = 0; i < N; ++i ) NAME[i] = lcc_label_new(_lcc)
#define USELBL(N)   USENELBL(L, N)
#define LABEL(LBL)  do{ lcc_label(_lcc, LBL);}while(0)
#define LABDA()     lcc_label_new(_lcc)
#define SPLIT(LBL)  do{ lcc_split(_lcc, LBL); }while(0)
#define SPLIR(LBL)  do{ lcc_splir(_lcc, LBL); }while(0)
#define JMP(LBL)    do{ lcc_jmp(_lcc, LBL); }while(0)
#define SAVE(ID)    do{ lcc_save(_lcc, ID); }while(0)
#define NODE(ID)    do{ lcc_node(_lcc, ID); }while(0)
#define PARENT()    do{ lcc_nodeex(_lcc, NOP_PARENT); }while(0)
#define NDISABLE()  do{ lcc_nodeex(_lcc, NOP_DISABLE); }while(0)
#define NENABLE()   do{ lcc_nodeex(_lcc, NOP_ENABLE); }while(0)
#define FN(N, STRE,ERR) for( int _tmp = lcc_fn_prolog(_lcc, N, strlen(N), STRE, ERR); _tmp; _tmp = lcc_fn_epilog(_lcc, STRE) )
#define CALLI(ID)   do{ lcc_calli(_lcc, ID); }while(0)
#define CALL(N)     do{ lcc_call(_lcc, N, strlen(N)); }while(0)
#define RET(ID)     do{ lcc_ret(_lcc); }while(0)
#define ERRADD(S)   ({ long r = lcc_error_add(_lcc, S, strlen(S)); if( r < 0 ) lcc_err_die(_lcc); r;})
#define ERROR(N)    do{ if( !lcc_error(_lcc, N) ) lcc_err_die(_lcc); } while(0)
#define START(INC)  do{ lcc_start(_lcc, INC); }while(0)
#define SEMFASE()   do{ lcc_semantic_fase_new(_lcc); }while(0)
#define SEMRULE()   do{ lcc_semantic_rule_new(_lcc); }while(0)
#define ENTER(N)    do{ lcc_enter(_lcc, N, strlen(N)); }while(0)
#define TYPE(N)     do{ lcc_type(_lcc, N, strlen(N)); }while(0)
#define LEAVE(N)    do{ lcc_leave(_lcc); }while(0)
#define VALUE(ST,N) do{ lcc_value(_lcc, ST, N, strlen(N)); } while(0)
#define SYMBOL(ST,N)  do{ lcc_symbol(_lcc, N, strlen(N)); } while(0)
#define SCOPENEW()    do{ lcc_scope(_lcc, 0); } while(0)
#define SCOPELEAVE()  do{ lcc_scope(_lcc, 1); } while(0)
#define SCOPESYMBOL() do{ lcc_scope(_lcc, 2); } while(0)
#define MAKE(SET)     do{ SET = lcc_make(_lcc); if( !SET ) lcc_err_die(_lcc); }while(0)

#define OR2(CMDSA, CMDSB) do{\
	USENELBL(LNOR,2);\
	SPLIT(LNOR[1]);\
	CMDSA\
	JMP(LNOR[0]);\
	LABEL(LNOR[1]);\
	CMDSB\
	LABEL(LNOR[0]);\
}while(0)

#define OR3(CMDSA, CMDSB, CMDSC) do{\
	USENELBL(LNOR,3);\
	SPLIT(LNOR[1]);\
	CMDSA\
	JMP(LNOR[0]);\
	LABEL(LNOR[1]);\
	SPLIT(LNOR[2]);\
	CMDSB\
	JMP(LNOR[0]);\
	LABEL(LNOR[2]);\
	CMDSC\
	LABEL(LNOR[0]);\
}while(0)

#define CHOOSE_BEGIN(N) do{\
	unsigned _incor = 1;\
	unsigned const _maxcor = N;\
	USENELBL(_choose, N);\
	SPLIT(_choose[_incor])

#define CHOOSE() do{\
	if( _incor < _maxcor-1 ){\
		JMP(_choose[0]);\
		LABEL(_choose[_incor++]);\
		SPLIT(_choose[_incor]);\
	}\
	else{\
		JMP(_choose[0]);\
		LABEL(_choose[_incor]);\
	}\
}while(0)

#define CHOOSE_END() LABEL(_choose[0]);\
}while(0)

//?
#define ZOQ(CMD) do{\
	USENELBL(LZ, 1);\
	SPLIT(LZ[0]);\
	CMD\
	LABEL(LZ[0]);\
}while(0)

//*
#define ZMQ(CMD) do{\
	USENELBL(LZM, 2);\
	LABEL(LZM[1]);\
	SPLIT(LZM[0]);\
	CMD\
	JMP(LZM[1]);\
	LABEL(LZM[0]);\
}while(0)

//+
#define OMQ(CMD) do{\
	USENELBL(LOM, 1);\
	LABEL(LOM[0]);\
	CMD\
	SPLIR(LOM[0]);\
}while(0)





#endif
