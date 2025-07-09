#ifndef __MFM_RECOM_H__
#define __MFM_RECOM_H__

#include <mfmrevm.h>

typedef struct reclabel{
	long      address;
	unsigned* resolve;
}reclabel_s;

typedef struct recrange{
	uint16_t map[16];
}recrange_s;

typedef struct recurange{
	uint16_t map[16];
	uint16_t uni[512];
}recurange_s;

typedef struct recfn{
	unsigned addr;
	char*    name;
	unsigned fail;
}recfn_s;

typedef struct recom{
	uint16_t*    bytecode;
	recfn_s*     fn;
	recfn_s*     call;
	reclabel_s*  label;
	recrange_s*  range;
	recurange_s* urange;
	unsigned     currentFN;
}recom_s;

recom_s* recom_ctor(recom_s* rc);
void recom_dtor(recom_s* rc);
recom_s* recom_match(recom_s* rc);
recom_s* recom_char(recom_s* rc, uint8_t val);
recom_s* recom_utf8(recom_s* rc, const utf8_t* val);
recrange_s* recom_range_ctor(recrange_s* range);
recrange_s* recom_range_set(recrange_s* range, uint8_t ch);
recrange_s* recom_range_clr(recrange_s* range, uint8_t ch);
recrange_s* recom_range_reverse(recrange_s* range);
unsigned recom_range_add(recom_s* rec, recrange_s* range);
recom_s* recom_range(recom_s* rc, uint16_t val);
unsigned recom_label_new(recom_s* rc);
unsigned recom_label(recom_s* rc, unsigned lbl);
recom_s* recom_split(recom_s* rc, unsigned lbl);
recom_s* recom_splir(recom_s* rc, unsigned lbl);
recom_s* recom_jmp(recom_s* rc, unsigned lbl);
recom_s* recom_save(recom_s* rc, uint8_t id);
recom_s* recom_node(recom_s* rc, uint16_t id);
unsigned recom_fn(recom_s* rc, const char* name, unsigned len);
recom_s* recom_fn_prolog(recom_s* rc, const char* name, unsigned store);
recom_s* recom_fn_epilog(recom_s* rc, unsigned stored);
recom_s* recom_calli(recom_s* rc, unsigned ifn);
recom_s* recom_call(recom_s* rc, const char* name, unsigned len);
recom_s* recom_ret(recom_s* rc, int fn);
recom_s* recom_parent(recom_s* rc);
recom_s* recom_start(recom_s* rc, int search);
uint16_t* recom_make(recom_s* rc);

#define CTOR()      recom_s RECOBJ; recom_ctor(&RECOBJ)
#define INIT(R)     recom_s* RECOM = R
#define ROBJ()      &RECOBJ
#define DTOR()      do{ recom_dtor(RECOM); }while(0)
#define MATCH()     do{ recom_match(RECOM); }while(0)
#define CHAR(CH)    do{ recom_char(RECOM, CH); }while(0)
#define UNI(UTF8)   do{ recom_utf8(RECOM, UTF8); }while(0)
#define USERANGE(R) recrange_s TMPRANGE
#define RRANGE()    do{ recom_range_ctor(&TMPRANGE); }while(0)
#define RRSET(CH)   do{ recom_range_set(&TMPRANGE, CH); }while(0)
#define RRREV(CH)   do{ recom_range_reverse(&TMPRANGE); }while(0)
#define RRADD()     recom_range_add(RECOM, &TMPRANGE)
#define RANGE(ID)   do{ recom_range(RECOM, ID); }while(0)
#define USELBL(N)   unsigned L[N]; for( unsigned i = 0; i < N; ++i ) L[i] = recom_label_new(RECOM)
#define LABEL(LBL)  do{ recom_label(RECOM, LBL);}while(0)
#define LABDA()     recom_label_new(RECOM)
#define SPLIT(LBL)  do{ recom_split(RECOM, LBL); }while(0)
#define SPLIR(LBL)  do{ recom_splir(RECOM, LBL); }while(0)
#define JMP(LBL)    do{ recom_jmp(RECOM, LBL); }while(0)
#define SAVE(ID)    do{ recom_save(RECOM, ID); }while(0)
#define NODE(ID)    do{ recom_node(RECOM, ID); }while(0)
#define FN(N, STRE) do{ recom_fn_prolog(RECOM, N, STRE);} while(0)
#define CALLI(ID)   do{ recom_calli(RECOM, ID); }while(0)
#define CALL(N,L)   do{ recom_call(RECOM, N, L); }while(0)
#define RET(STRE)   do{ recom_fn_epilog(RECOM, STRE); }while(0)
#define RETI(ID)    do{ recom_ret(RECOM,ID); }while(0)
#define PARENT()    do{ recom_parent(RECOM); }while(0)
#define START(S)    do{ recom_start(RECOM, S); }while(0)
#define MAKE()      recom_make(RECOM)











#endif
