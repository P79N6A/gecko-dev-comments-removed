




































#include "VisitInfo.h"
#include "nsIURI.h"

namespace mozilla {
namespace places {




VisitInfo::VisitInfo(PRInt64 aVisitId,
                     PRTime aVisitDate,
                     PRUint32 aTransitionType,
                     already_AddRefed<nsIURI> aReferrer,
                     PRInt64 aSessionId)
: mVisitId(aVisitId)
, mVisitDate(aVisitDate)
, mTransitionType(aTransitionType)
, mReferrer(aReferrer)
, mSessionId(aSessionId)
{
}




NS_IMETHODIMP
VisitInfo::GetVisitId(PRInt64* _visitId)
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
VisitInfo::GetTransitionType(PRUint32* _transitionType)
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

NS_IMETHODIMP
VisitInfo::GetSessionId(PRInt64* _sessionId)
{
  *_sessionId = mSessionId;
  return NS_OK;
}




NS_IMPL_ISUPPORTS1(
  VisitInfo
, mozIVisitInfo
)

} 
} 
