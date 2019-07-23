




































#ifndef nsTraceRefcntImpl_h___
#define nsTraceRefcntImpl_h___

#include <stdio.h> 
#include "nsITraceRefcnt.h"

class nsTraceRefcntImpl : public nsITraceRefcnt
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSITRACEREFCNT

  static NS_COM void Startup();  
  static NS_COM void Shutdown();

  enum StatisticsType {
    ALL_STATS,
    NEW_STATS
  };

  static NS_COM nsresult DumpStatistics(StatisticsType type = ALL_STATS,
                                        FILE* out = 0);
  
  static NS_COM void ResetStatistics(void);

  static NS_COM void DemangleSymbol(const char * aSymbol, 
                                    char * aBuffer,
                                    int aBufLen);

  static NS_COM void WalkTheStack(FILE* aStream);
  




  static NS_COM void SetActivityIsLegal(PRBool aLegal);

  static NS_METHOD Create(nsISupports* outer, const nsIID& aIID, void* *aInstancePtr);
};

#define NS_TRACE_REFCNT_CONTRACTID "@mozilla.org/xpcom/trace-refcnt;1"
#define NS_TRACE_REFCNT_CLASSNAME  "nsTraceRefcnt Interface"
#define NS_TRACE_REFCNT_CID                          \
{ /* e3e7511e-a395-4924-94b1-d527861cded4 */         \
    0xe3e7511e,                                      \
    0xa395,                                          \
    0x4924,                                          \
    {0x94, 0xb1, 0xd5, 0x27, 0x86, 0x1c, 0xde, 0xd4} \
}                                                    \




extern "C" NS_COM void 
NS_MeanAndStdDev(double n, double sumOfValues, double sumOfSquaredValues,
                 double *meanResult, double *stdDevResult);


#endif
