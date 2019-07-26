



#include "VisitInfo.h"
#include "nsIURI.h"

namespace mozilla {
namespace places {




VisitInfo::VisitInfo(int64_t aVisitId,
                     PRTime aVisitDate,
                     uint32_t aTransitionType,
                     already_AddRefed<nsIURI> aReferrer)
: mVisitId(aVisitId)
, mVisitDate(aVisitDate)
, mTransitionType(aTransitionType)
, mReferrer(aReferrer)
{
}




NS_IMETHODIMP
VisitInfo::GetVisitId(int64_t* _visitId)
{
  *_visitId = mVisitId;
  return NS_OK;
}

NS_IMETHODIMP
VisitInfo::GetVisitDate(PRTime* _visitDate)
{
  *_visitDate = mVisitDate;
  return NS_OK;
}

NS_IMETHODIMP
VisitInfo::GetTransitionType(uint32_t* _transitionType)
{
  *_transitionType = mTransitionType;
  return NS_OK;
}

NS_IMETHODIMP
VisitInfo::GetReferrerURI(nsIURI** _referrer)
{
  NS_IF_ADDREF(*_referrer = mReferrer);
  return NS_OK;
}




NS_IMPL_ISUPPORTS1(
  VisitInfo
, mozIVisitInfo
)

} 
} 
