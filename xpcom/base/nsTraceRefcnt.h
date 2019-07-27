




#ifndef nsTraceRefcnt_h___
#define nsTraceRefcnt_h___

#include <stdio.h> 
#include "nscore.h"

class nsTraceRefcnt
{
public:
  static void Shutdown();

  enum StatisticsType {
    ALL_STATS,
    NEW_STATS
  };

  static nsresult DumpStatistics(StatisticsType aType = ALL_STATS,
                                 FILE* aOut = 0);

  static void ResetStatistics();

  static void DemangleSymbol(const char* aSymbol, char* aBuffer, int aBufLen);

  static void WalkTheStack(FILE* aStream);

  







  static void WalkTheStackCached(FILE* aStream);

  




  static void SetActivityIsLegal(bool aLegal);
};

#define NS_TRACE_REFCNT_CONTRACTID "@mozilla.org/xpcom/trace-refcnt;1"
#define NS_TRACE_REFCNT_CID                          \
{ /* e3e7511e-a395-4924-94b1-d527861cded4 */         \
    0xe3e7511e,                                      \
    0xa395,                                          \
    0x4924,                                          \
    {0x94, 0xb1, 0xd5, 0x27, 0x86, 0x1c, 0xde, 0xd4} \
}                                                    \




extern "C" void
NS_MeanAndStdDev(double aNumberOfValues,
                 double aSumOfValues, double aSumOfSquaredValues,
                 double* aMeanResult, double* aStdDevResult);


#endif
