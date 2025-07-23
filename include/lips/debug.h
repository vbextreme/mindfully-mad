#ifndef __LIPS_DEBUG_H__
#define __LIPS_DEBUG_H__

#include <lips/vm.h>

#define LIPS_DBG_X         0
#define LIPS_DBG_Y         0
#define LIPS_DBG_SCREEN_W  120
#define LIPS_DBG_HEADER_H  3
#define LIPS_DBG_HEADER_W  LIPS_DBG_SCREEN_W
#define LIPS_DBG_STACKED_H 7
#define LIPS_DBG_STACK_W   29
#define LIPS_DBG_CSTACK_W  14
#define LIPS_DBG_NODE_W    54
#define LIPS_DBG_CAPTURE_W 17
#define LIPS_DBG_BREAK_W   10
#define LIPS_DBG_RANGE_H   3
#define LIPS_DBG_RANGE_W   LIPS_DBG_SCREEN_W
#define LIPS_DBG_CODE_H    17
#define LIPS_DBG_CODE_W    LIPS_DBG_SCREEN_W
#define LIPS_DBG_PROMPT_H  3
#define LIPS_DBG_PROMPT_W  LIPS_DBG_SCREEN_W
#define LIPS_DBG_INP_H     4
#define LIPS_DBG_INP_W     LIPS_DBG_SCREEN_W

void lips_vm_debug(lipsVM_s* vm);
void lips_dump_error(lipsVM_s* vm, lipsMatch_s* m, const utf8_t* source, FILE* f);
void lips_dump_capture(lipsMatch_s* m, FILE* f);
void lips_dump_ast(lipsVM_s* vm, FILE* f, int mode);

#endif
