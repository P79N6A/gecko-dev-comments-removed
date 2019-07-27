




#ifndef NS_ANDROIDHISTORY_H
#define NS_ANDROIDHISTORY_H

#include "IHistory.h"
#include "nsDataHashtable.h"
#include "nsTPriorityQueue.h"
#include "nsIRunnable.h"
#include "nsIURI.h"

#define NS_ANDROIDHISTORY_CID \
    {0xCCAA4880, 0x44DD, 0x40A7, {0xA1, 0x3F, 0x61, 0x56, 0xFC, 0x88, 0x2C, 0x0B}}


#define RECENTLY_VISITED_URI_SIZE 8


#define EMBED_URI_SIZE 128

class nsAndroidHistory MOZ_FINAL : public mozilla::IHistory, public nsIRunnable
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_IHISTORY
  NS_DECL_NSIRUNNABLE

  



  static nsAndroidHistory* GetSingleton();

  nsAndroidHistory();

private:
  ~nsAndroidHistory() {}

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

  



  typedef nsAutoTArray<nsCOMPtr<nsIURI>, EMBED_URI_SIZE> EmbedArray;
  EmbedArray::index_type mEmbedURIsNextIndex;
  EmbedArray mEmbedURIs;

  void AppendToEmbedURIs(nsIURI* aURI);
  bool IsEmbedURI(nsIURI* aURI);
};

#endif
