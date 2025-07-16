#ifndef __LIPS_DEBUG_H__
#define __LIPS_DEBUG_H__

#include <lips/vm.h>

#define LIPS_DBG_HEADER_H  1
#define LIPS_DBG_STACKED_H 5
#define LIPS_DBG_RANGE_H   1
#define LIPS_DBG_CODE_H    20
#define LIPS_DBG_PROMPT_H  1
#define LIPS_DBG_INP_H     2
#define LIPS_DBG_SCREEN_W  120
#define LIPS_DBG_STACK_W   (DBG_SCREEN_W/4)
#define LIPS_DBG_CSTACK_W  (DBG_SCREEN_W/4-1)
#define LIPS_DBG_NODE_W    (DBG_SCREEN_W/4-1)
#define LIPS_DBG_CAPTURE_W (DBG_SCREEN_W/4-1)


void lips_dump_error(lipsMatch_s* m, const utf8_t* source, FILE* f);


#endif
