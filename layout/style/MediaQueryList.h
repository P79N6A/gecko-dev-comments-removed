






#ifndef mozilla_dom_MediaQueryList_h
#define mozilla_dom_MediaQueryList_h

#include "nsIDOMMediaQueryList.h"
#include "nsCycleCollectionParticipant.h"
#include "nsAutoPtr.h"
#include "nsCOMPtr.h"
#include "nsTArray.h"
#include "prclist.h"
#include "mozilla/Attributes.h"
#include "nsWrapperCache.h"
#include "mozilla/dom/MediaQueryListBinding.h"

class nsPresContext;
class nsMediaList;

namespace mozilla {
namespace dom {

class MediaQueryList MOZ_FINAL : public nsIDOMMediaQueryList,
                                 public nsWrapperCache,
                                 public PRCList
{
public:
  
  
  MediaQueryList(nsPresContext *aPresContext,
                 const nsAString &aMediaQueryList);
private:
  ~MediaQueryList();

public:
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS(MediaQueryList)

  NS_DECL_NSIDOMMEDIAQUERYLIST

  nsISupports* GetParentObject() const;

  typedef CallbackObjectHolder<mozilla::dom::MediaQueryListListener,
                               nsIDOMMediaQueryListListener> CallbackType;

  struct HandleChangeData {
    nsRefPtr<MediaQueryList> mql;
    nsCOMPtr<nsIDOMMediaQueryListListener> listener;
    nsCOMPtr<mozilla::dom::MediaQueryListListener> callback;
  };

  typedef FallibleTArray< nsCOMPtr<nsIDOMMediaQueryListListener> > ListenerList;
  typedef FallibleTArray< nsRefPtr<mozilla::dom::MediaQueryListListener> > CallbackList;
  typedef FallibleTArray<HandleChangeData> NotifyList;

  
  void MediumFeaturesChanged(NotifyList &aListenersToNotify);

  bool HasListeners() const { return !mListeners.IsEmpty() || !mCallbacks.IsEmpty(); }

  void RemoveAllListeners();

  JSObject* WrapObject(JSContext* aCx, JS::Handle<JSObject*> aScope) MOZ_OVERRIDE;

  
  
  bool Matches();
  void AddListener(mozilla::dom::MediaQueryListListener& aListener);
  void RemoveListener(mozilla::dom::MediaQueryListListener& aListener);

private:
  void RecomputeMatches();

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  nsRefPtr<nsPresContext> mPresContext;

  nsRefPtr<nsMediaList> mMediaList;
  bool mMatches;
  bool mMatchesValid;
  ListenerList mListeners;
  CallbackList mCallbacks;
};

} 
} 

#endif 
