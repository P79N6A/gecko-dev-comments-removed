




































#ifndef mozilla_places_PlaceInfo_h__
#define mozilla_places_PlaceInfo_h__

#include "mozIAsyncHistory.h"
#include "nsString.h"
#include "nsAutoPtr.h"

class nsIURI;
class mozIVisitInfo;

namespace mozilla {
namespace places {


class PlaceInfo : public mozIPlaceInfo
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_MOZIPLACEINFO

  typedef nsTArray< nsCOMPtr<mozIVisitInfo> > VisitsArray;

  PlaceInfo(PRInt64 aId, const nsCString& aGUID, already_AddRefed<nsIURI> aURI,
            const nsString& aTitle, PRInt64 aFrecency,
            const VisitsArray& aVisits);

private:
  const PRInt64 mId;
  const nsCString mGUID;
  nsCOMPtr<nsIURI> mURI;
  const nsString mTitle;
  const PRInt64 mFrecency;
  const VisitsArray mVisits;
};

} 
} 

#endif 
