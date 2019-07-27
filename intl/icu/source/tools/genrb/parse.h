
















#ifndef PARSE_H
#define PARSE_H 1

#include "unicode/utypes.h"
#include "filestrm.h"
#include "ucbuf.h"

U_CDECL_BEGIN

void initParser();


struct SRBRoot* parse(UCHARBUF *buf, const char* inputDir, const char* outputDir,
                      const char *filename,
                      UBool makeBinaryCollation, UBool omitCollationRules, UErrorCode *status);

U_CDECL_END

#endif
