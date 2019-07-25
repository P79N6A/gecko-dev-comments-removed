






































#ifndef nsDOMMediaQueryList_h_
#define nsDOMMediaQueryList_h_

#include "nsIDOMMediaQueryList.h"
#include "nsCycleCollectionParticipant.h"
#include "nsAutoPtr.h"
#include "nsCOMPtr.h"
#include "nsTArray.h"
#include "prclist.h"

class nsPresContext;
class nsMediaList;

class nsDOMMediaQueryList : public nsIDOMMediaQueryList,
                            public PRCList
{
public:
  
  
  nsDOMMediaQueryList(nsPresContext *aPresContext,
                      const nsAString &aMediaQueryList);
private:
  ~nsDOMMediaQueryList();

public:
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_CLASS(nsDOMMediaQueryList)

  NS_DECL_NSIDOMMEDIAQUERYLIST

  struct HandleChangeData {
    nsRefPtr<nsDOMMediaQueryList> mql;
    nsCOMPtr<nsIDOMMediaQueryListListener> listener;
  };

  typedef FallibleTArray< nsCOMPtr<nsIDOMMediaQueryListListener> > ListenerList;
  typedef FallibleTArray<HandleChangeData> NotifyList;

  
  void MediumFeaturesChanged(NotifyList &aListenersToNotify);

private:
  void RecomputeMatches();

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  nsRefPtr<nsPresContext> mPresContext;

  nsRefPtr<nsMediaList> mMediaList;
  PRPackedBool mMatches;
  PRPackedBool mMatchesValid;
  ListenerList mListeners;
};

#endif 
