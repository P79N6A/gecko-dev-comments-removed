




































#ifndef NS_ANDROIDHISTORY_H
#define NS_ANDROIDHISTORY_H

#include "IHistory.h"
#include "nsDataHashtable.h"
#include "nsTPriorityQueue.h"
#include "nsThreadUtils.h"

#define NS_ANDROIDHISTORY_CID \
    {0xCCAA4880, 0x44DD, 0x40A7, {0xA1, 0x3F, 0x61, 0x56, 0xFC, 0x88, 0x2C, 0x0B}}

class nsAndroidHistory : public mozilla::IHistory, public nsIRunnable
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_IHISTORY
  NS_DECL_NSIRUNNABLE

  



  static nsAndroidHistory* GetSingleton();

  nsAndroidHistory();
  static void NotifyURIVisited(const nsString& str);

private:
  static nsAndroidHistory* sHistory;

  nsDataHashtable<nsStringHashKey, nsTArray<mozilla::dom::Link *> *> mListeners;
  nsTPriorityQueue<nsString> mPendingURIs;
};

#endif
