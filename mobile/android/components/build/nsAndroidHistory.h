




#ifndef NS_ANDROIDHISTORY_H
#define NS_ANDROIDHISTORY_H

#include "IHistory.h"
#include "nsDataHashtable.h"
#include "nsTPriorityQueue.h"
#include "nsIRunnable.h"

#define NS_ANDROIDHISTORY_CID \
    {0xCCAA4880, 0x44DD, 0x40A7, {0xA1, 0x3F, 0x61, 0x56, 0xFC, 0x88, 0x2C, 0x0B}}


#define RECENTLY_VISITED_URI_SIZE 8

class nsAndroidHistory : public mozilla::IHistory, public nsIRunnable
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_IHISTORY
  NS_DECL_NSIRUNNABLE

  



  static nsAndroidHistory* GetSingleton();

  nsAndroidHistory();

private:
  static nsAndroidHistory* sHistory;

  nsDataHashtable<nsStringHashKey, nsTArray<mozilla::dom::Link *> *> mListeners;
  nsTPriorityQueue<nsString> mPendingURIs;

  nsresult CanAddURI(nsIURI* aURI, bool* canAdd);

  



  typedef nsAutoTArray<nsCOMPtr<nsIURI>, RECENTLY_VISITED_URI_SIZE>
          RecentlyVisitedArray;
  RecentlyVisitedArray mRecentlyVisitedURIs;
  RecentlyVisitedArray::index_type mRecentlyVisitedURIsNextIndex;

  void AppendToRecentlyVisitedURIs(nsIURI* aURI);
  bool IsRecentlyVisitedURI(nsIURI* aURI);
};

#endif
