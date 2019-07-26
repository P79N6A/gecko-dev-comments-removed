






#ifndef mozilla_dom_MediaQueryList_h
#define mozilla_dom_MediaQueryList_h

#include "nsIDOMMediaQueryList.h"
#include "nsCycleCollectionParticipant.h"
#include "nsAutoPtr.h"
#include "nsCOMPtr.h"
#include "nsTArray.h"
#include "prclist.h"
#include "mozilla/Attributes.h"

class nsPresContext;
class nsMediaList;

namespace mozilla {
namespace dom {

class MediaQueryList MOZ_FINAL : public nsIDOMMediaQueryList,
                                 public PRCList
{
public:
  
  
  MediaQueryList(nsPresContext *aPresContext,
                 const nsAString &aMediaQueryList);
private:
  ~MediaQueryList();

public:
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_CLASS(MediaQueryList)

  NS_DECL_NSIDOMMEDIAQUERYLIST

  struct HandleChangeData {
    nsRefPtr<MediaQueryList> mql;
    nsCOMPtr<nsIDOMMediaQueryListListener> listener;
  };

  typedef FallibleTArray< nsCOMPtr<nsIDOMMediaQueryListListener> > ListenerList;
  typedef FallibleTArray<HandleChangeData> NotifyList;

  
  void MediumFeaturesChanged(NotifyList &aListenersToNotify);

  bool HasListeners() const { return !mListeners.IsEmpty(); }

  void RemoveAllListeners();

private:
  void RecomputeMatches();

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  nsRefPtr<nsPresContext> mPresContext;

  nsRefPtr<nsMediaList> mMediaList;
  bool mMatches;
  bool mMatchesValid;
  ListenerList mListeners;
};

} 
} 

#endif 
