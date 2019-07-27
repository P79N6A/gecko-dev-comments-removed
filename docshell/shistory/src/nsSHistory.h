





#ifndef nsSHistory_h
#define nsSHistory_h


#include "nsCOMPtr.h"


#include "nsISHistory.h"
#include "nsISHistoryInternal.h"
#include "nsIWebNavigation.h"
#include "nsISimpleEnumerator.h"
#include "nsTObserverArray.h"
#include "nsWeakPtr.h"


#include "prclist.h"

class nsIDocShell;
class nsSHEnumerator;
class nsSHistoryObserver;
class nsISHEntry;
class nsISHTransaction;

class nsSHistory MOZ_FINAL : public PRCList,
                             public nsISHistory,
                             public nsISHistoryInternal,
                             public nsIWebNavigation
{
public:
  nsSHistory();

  NS_DECL_ISUPPORTS
  NS_DECL_NSISHISTORY
  NS_DECL_NSISHISTORYINTERNAL
  NS_DECL_NSIWEBNAVIGATION

  
  static nsresult Startup();
  static void Shutdown();
  static void UpdatePrefs();

  
  
  
  
  static uint32_t GetMaxTotalViewers() { return sHistoryMaxTotalViewers; }

protected:
  virtual ~nsSHistory();
  friend class nsSHEnumerator;
  friend class nsSHistoryObserver;

   
   NS_IMETHOD GetTransactionAtIndex(int32_t aIndex, nsISHTransaction ** aResult);
   nsresult CompareFrames(nsISHEntry * prevEntry, nsISHEntry * nextEntry, nsIDocShell * rootDocShell, long aLoadType, bool * aIsFrameFound);
   nsresult InitiateLoad(nsISHEntry * aFrameEntry, nsIDocShell * aFrameDS, long aLoadType);

   NS_IMETHOD LoadEntry(int32_t aIndex, long aLoadType, uint32_t histCmd);

#ifdef DEBUG
   nsresult PrintHistory();
#endif

  
  
  void EvictOutOfRangeWindowContentViewers(int32_t aIndex);
  static void GloballyEvictContentViewers();
  static void GloballyEvictAllContentViewers();

  
  
  static uint32_t CalcMaxTotalViewers();

  void RemoveDynEntries(int32_t aOldIndex, int32_t aNewIndex);

  nsresult LoadNextPossibleEntry(int32_t aNewIndex, long aLoadType, uint32_t aHistCmd);
protected:
  
  
  
  bool RemoveDuplicate(int32_t aIndex, bool aKeepNext);

  nsCOMPtr<nsISHTransaction> mListRoot;
  int32_t mIndex;
  int32_t mLength;
  int32_t mRequestedIndex;
  
  nsAutoTObserverArray<nsWeakPtr, 2> mListeners;
  
  nsIDocShell *  mRootDocShell;

  
  static int32_t  sHistoryMaxTotalViewers;
};



class nsSHEnumerator : public nsISimpleEnumerator
{
public:

  NS_DECL_ISUPPORTS
  NS_DECL_NSISIMPLEENUMERATOR

  explicit nsSHEnumerator(nsSHistory *  aHistory);
  
protected:
  friend class nsSHistory;
  virtual ~nsSHEnumerator();
private:
  int32_t     mIndex;
  nsSHistory *  mSHistory;  
};

#endif   
