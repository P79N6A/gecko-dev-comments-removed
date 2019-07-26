
















#ifndef PARSE_H
#define PARSE_H 1

#include "unicode/utypes.h"
#include "filestrm.h"
#include "ucbuf.h"

U_CDECL_BEGIN

void initParser(UBool omitCollationRules);


struct SRBRoot* parse(UCHARBUF *buf, const char* inputDir, const char* outputDir,
                      UBool omitBinaryCollation, UErrorCode *status);

U_CDECL_END

#endif
