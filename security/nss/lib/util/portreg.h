














































































 
#ifndef SHEXP_H
#define SHEXP_H

#include "utilrename.h"

















#define NON_SXP -1
#define INVALID_SXP -2
#define VALID_SXP 1

SEC_BEGIN_PROTOS

extern int PORT_RegExpValid(const char *exp);

extern int PORT_RegExpSearch(const char *str, const char *exp);


extern int PORT_RegExpCaseSearch(const char *str, const char *exp);

SEC_END_PROTOS

#endif
