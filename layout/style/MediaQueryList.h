







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

class MediaQueryList MOZ_FINAL : public nsISupports,
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

  typedef FallibleTArray< nsRefPtr<mozilla::dom::MediaQueryListListener> > CallbackList;
  typedef FallibleTArray<HandleChangeData> NotifyList;

  
  void MediumFeaturesChanged(NotifyList &aListenersToNotify);

  bool HasListeners() const { return !mCallbacks.IsEmpty(); }

  void RemoveAllListeners();

  JSObject* WrapObject(JSContext* aCx) MOZ_OVERRIDE;

  
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
  CallbackList mCallbacks;
};

} 
} 

#endif 
