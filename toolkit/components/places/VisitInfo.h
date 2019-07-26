



#ifndef mozilla_places_VisitInfo_h__
#define mozilla_places_VisitInfo_h__

#include "mozIAsyncHistory.h"
#include "nsAutoPtr.h"
#include "mozilla/Attributes.h"

class nsIURI;

namespace mozilla {
namespace places {

class VisitInfo MOZ_FINAL : public mozIVisitInfo
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_MOZIVISITINFO

  VisitInfo(int64_t aVisitId, PRTime aVisitDate, uint32_t aTransitionType,
            already_AddRefed<nsIURI> aReferrer);

private:
  const int64_t mVisitId;
  const PRTime mVisitDate;
  const uint32_t mTransitionType;
  nsCOMPtr<nsIURI> mReferrer;
};

} 
} 

#endif 
