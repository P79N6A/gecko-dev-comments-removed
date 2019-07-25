



#include "nsIOpenWindowEventDetail.h"
#include "nsIDOMNode.h"
#include "nsCycleCollectionParticipant.h"
#include "nsCOMPtr.h"
#include "nsString.h"





class nsOpenWindowEventDetail : public nsIOpenWindowEventDetail
{
public:
  nsOpenWindowEventDetail(const nsAString& aURL,
                          const nsAString& aName,
                          const nsAString& aFeatures,
                          nsIDOMNode* aFrameElement)
    : mURL(aURL)
    , mName(aName)
    , mFeatures(aFeatures)
    , mFrameElement(aFrameElement)
  {}

  virtual ~nsOpenWindowEventDetail() {}

  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_CLASS(nsOpenWindowEventDetail)
  NS_DECL_NSIOPENWINDOWEVENTDETAIL

private:
  const nsString mURL;
  const nsString mName;
  const nsString mFeatures;
  nsCOMPtr<nsIDOMNode> mFrameElement;
};
