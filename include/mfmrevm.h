#ifndef __MFM_REVM_H__
#define __MFM_REVM_H__

#include <notstd/core.h>
#include <notstd/utf8.h>

#define REVM_MAX_CAPTURE 128

typedef struct revmMatch{
	const utf8_t* capture[REVM_MAX_CAPTURE * 2];
	int match;
}revmMatch_s;

revmMatch_s revm_match(const utf8_t* txt, const uint16_t* bytecode);



#endif

