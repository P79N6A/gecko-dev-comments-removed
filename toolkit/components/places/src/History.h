






































#ifndef mozilla_places_History_h_
#define mozilla_places_History_h_

#include "mozilla/IHistory.h"
#include "mozilla/dom/Link.h"
#include "nsTHashtable.h"
#include "nsString.h"
#include "nsURIHashKey.h"
#include "nsTArray.h"
#include "nsDeque.h"
#include "nsIObserver.h"

namespace mozilla {
namespace places {

#define NS_HISTORYSERVICE_CID \
  {0x9fc91e65, 0x1475, 0x4353, {0x9b, 0x9a, 0x93, 0xd7, 0x6f, 0x5b, 0xd9, 0xb7}}

class History : public IHistory
              , public nsIObserver
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_IHISTORY
  NS_DECL_NSIOBSERVER

  History();

  





  void NotifyVisited(nsIURI *aURI);

  








  void AppendTask(class Step* aTask);

  






  void CurrentTaskFinished();

  


  static History *GetService();

  



  static History *GetSingleton();

private:
  ~History();

  











  nsDeque mPendingVisits;

  



  void StartNextTask();

  static History *gService;

  
  bool mShuttingDown;

  typedef nsTArray<mozilla::dom::Link *> ObserverArray;

  class KeyClass : public nsURIHashKey
  {
  public:
    KeyClass(const nsIURI *aURI)
    : nsURIHashKey(aURI)
    {
    }
    KeyClass(const KeyClass &aOther)
    : nsURIHashKey(aOther)
    {
      NS_NOTREACHED("Do not call me!");
    }
    ObserverArray array;
  };

  nsTHashtable<KeyClass> mObservers;
};

} 
} 

#endif 
