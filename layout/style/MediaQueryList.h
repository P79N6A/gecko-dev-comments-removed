







#ifndef mozilla_dom_MediaQueryList_h
#define mozilla_dom_MediaQueryList_h

#include "nsISupports.h"
#include "nsCycleCollectionParticipant.h"
#include "nsAutoPtr.h"
#include "nsCOMPtr.h"
#include "nsTArray.h"
#include "prclist.h"
#include "mozilla/Attributes.h"
#include "nsWrapperCache.h"
#include "mozilla/dom/MediaQueryListBinding.h"

class nsIDocument;
class nsMediaList;

namespace mozilla {
namespace dom {

class MediaQueryList final : public nsISupports,
                             public nsWrapperCache,
                             public PRCList
{
public:
  
  
  MediaQueryList(nsIDocument *aDocument,
                 const nsAString &aMediaQueryList);
private:
  ~MediaQueryList();

public:
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS(MediaQueryList)

  nsISupports* GetParentObject() const;

  struct HandleChangeData {
    nsRefPtr<MediaQueryList> mql;
    nsRefPtr<mozilla::dom::MediaQueryListListener> callback;
  };

  
  void MediumFeaturesChanged(nsTArray<HandleChangeData>& aListenersToNotify);

  bool HasListeners() const { return !mCallbacks.IsEmpty(); }

  void RemoveAllListeners();

  JSObject* WrapObject(JSContext* aCx, JS::Handle<JSObject*> aGivenProto) override;

  
  void GetMedia(nsAString& aMedia);
  bool Matches();
  void AddListener(mozilla::dom::MediaQueryListListener& aListener);
  void RemoveListener(mozilla::dom::MediaQueryListListener& aListener);

private:
  void RecomputeMatches();

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  nsCOMPtr<nsIDocument> mDocument;

  nsRefPtr<nsMediaList> mMediaList;
  bool mMatches;
  bool mMatchesValid;
  nsTArray<nsRefPtr<mozilla::dom::MediaQueryListListener>> mCallbacks;
};

} 
} 

#endif 
