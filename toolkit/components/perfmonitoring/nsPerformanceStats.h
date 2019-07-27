




#ifndef nsPerformanceStats_h
#define nsPerformanceStats_h

#include "nsIPerformanceStats.h"

class nsPerformanceStatsService : public nsIPerformanceStatsService
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIPERFORMANCESTATSSERVICE

  nsPerformanceStatsService();

private:
  virtual ~nsPerformanceStatsService();

protected:
};

#endif
