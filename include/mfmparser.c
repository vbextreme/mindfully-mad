#ifndef __MB_PARSER_H__
#define __MB_PARSER_H__

#include <mfmlexer.h>

typedef enum{ PRE_TYPE_TOKEN, PRE_TYPE_PRULE, PRE_TYPE_OP_EQ, PRE_TYPE_OP_OR, PRE_TYPE_OP_AND } preType_e;

typedef struct pRule pRule_s;

typedef struct prExpr{
	preType_e type;
	union{
		lexToken_s* token;
		pRule_s*    rule;
		char* string;
	};
}prExpr_s;

struct pRule{
	char* name;
};

typedef struct mbparser{
	lexToken_s* token;
	

}mbparser_s;





#endif
