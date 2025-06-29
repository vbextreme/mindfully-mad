#ifndef __MFM_REVM_H__
#define __MFM_REVM_H__

#include <notstd/core.h>
#include <notstd/utf8.h>

typedef struct revmMatch{
	const utf8_t** capture;
	int match;
}revmMatch_s;

revmMatch_s revm_match(const utf8_t* txt, const uint16_t* bytecode);



#endif

