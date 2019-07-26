


















#ifndef UCOL_BLD_H
#define UCOL_BLD_H

#ifdef UCOL_DEBUG
#include <stdio.h>
#include <stdlib.h>
#endif

#include "unicode/utypes.h"

#if !UCONFIG_NO_COLLATION


#include "ucol_imp.h"
#include "ucol_tok.h"
#include "ucol_wgt.h"

U_CFUNC
UCATableHeader *ucol_assembleTailoringTable(UColTokenParser *src, UErrorCode *status);

typedef struct {
  WeightRange ranges[7];
  int32_t noOfRanges;
  uint32_t byteSize; uint32_t start; uint32_t limit;
  int32_t maxCount;
  int32_t count;
  uint32_t current;
  uint32_t fLow; 
  uint32_t fHigh; 
} ucolCEGenerator;

U_CFUNC uint32_t U_EXPORT2 ucol_getCEStrengthDifference(uint32_t CE, uint32_t contCE, 
                                            uint32_t prevCE, uint32_t prevContCE);

U_INTERNAL int32_t U_EXPORT2 ucol_findReorderingEntry(const char* name);

 
#endif 

#endif
