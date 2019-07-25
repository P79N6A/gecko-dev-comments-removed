




































#ifndef mozilla_places_VisitInfo_h__
#define mozilla_places_VisitInfo_h__

#include "mozIAsyncHistory.h"
#include "nsAutoPtr.h"

class nsIURI;

namespace mozilla {
namespace places {

class VisitInfo : public mozIVisitInfo
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_MOZIVISITINFO

  VisitInfo(PRInt64 aVisitId, PRTime aVisitDate, PRUint32 aTransitionType,
            already_AddRefed<nsIURI> aReferrer, PRInt64 aSessionId);

private:
  const PRInt64 mVisitId;
  const PRTime mVisitDate;
  const PRUint32 mTransitionType;
  nsCOMPtr<nsIURI> mReferrer;
  const PRInt64 mSessionId;
};

} 
} 

#endif 
