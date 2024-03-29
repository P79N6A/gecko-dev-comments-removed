




#include "nsCycleCollectionParticipant.h"
#include "nsISelectionController.h"
#include "nsIController.h"
#include "nsIControllers.h"
#include "nsIObserver.h"
#include "nsUnicharUtils.h"
#include "nsIFind.h"
#include "nsIWebBrowserFind.h"
#include "nsWeakReference.h"
#include "nsISelection.h"
#include "nsIDOMRange.h"
#include "nsIDocShellTreeItem.h"
#include "nsITypeAheadFind.h"
#include "nsISound.h"

class nsIPresShell;
class nsPresContext;

#define TYPEAHEADFIND_NOTFOUND_WAV_URL \
        "chrome://global/content/notfound.wav"

class nsTypeAheadFind : public nsITypeAheadFind,
                        public nsIObserver,
                        public nsSupportsWeakReference
{
public:
  nsTypeAheadFind();

  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_NSITYPEAHEADFIND
  NS_DECL_NSIOBSERVER

  NS_DECL_CYCLE_COLLECTION_CLASS_AMBIGUOUS(nsTypeAheadFind, nsITypeAheadFind)

protected:
  virtual ~nsTypeAheadFind();

  nsresult PrefsReset();

  void SaveFind();
  void PlayNotFoundSound(); 
  nsresult GetWebBrowserFind(nsIDocShell *aDocShell,
                             nsIWebBrowserFind **aWebBrowserFind);

  void RangeStartsInsideLink(nsIDOMRange *aRange, nsIPresShell *aPresShell, 
                             bool *aIsInsideLink, bool *aIsStartingLink);

  void GetSelection(nsIPresShell *aPresShell, nsISelectionController **aSelCon, 
                    nsISelection **aDomSel);
  
  
  bool IsRangeVisible(nsIPresShell *aPresShell, nsPresContext *aPresContext,
                        nsIDOMRange *aRange, bool aMustBeVisible, 
                        bool aGetTopVisibleLeaf, nsIDOMRange **aNewRange,
                        bool *aUsesIndependentSelection);
  nsresult FindItNow(nsIPresShell *aPresShell, bool aIsLinksOnly,
                     bool aIsFirstVisiblePreferred, bool aFindPrev,
                     uint16_t* aResult);
  nsresult GetSearchContainers(nsISupports *aContainer,
                               nsISelectionController *aSelectionController,
                               bool aIsFirstVisiblePreferred,
                               bool aFindPrev, nsIPresShell **aPresShell,
                               nsPresContext **aPresContext);

  
  
  already_AddRefed<nsIPresShell> GetPresShell();

  void ReleaseStrongMemberVariables();

  
  nsString mTypeAheadBuffer;
  nsCString mNotFoundSoundURL;

  
  
  bool mStartLinksOnlyPref;
  bool mCaretBrowsingOn;
  bool mDidAddObservers;
  nsCOMPtr<nsIDOMElement> mFoundLink;     
  nsCOMPtr<nsIDOMElement> mFoundEditable; 
  nsCOMPtr<nsIDOMRange> mFoundRange;      
  nsCOMPtr<nsIDOMWindow> mCurrentWindow;
  
  
  uint32_t mLastFindLength;

  
  
  nsCOMPtr<nsISound> mSoundInterface;
  bool mIsSoundInitialized;
  
  
  nsCOMPtr<nsIDOMRange> mStartFindRange;
  nsCOMPtr<nsIDOMRange> mSearchRange;
  nsCOMPtr<nsIDOMRange> mStartPointRange;
  nsCOMPtr<nsIDOMRange> mEndPointRange;

  
  nsCOMPtr<nsIFind> mFind;

  bool mCaseSensitive;

  bool EnsureFind() {
    if (mFind) {
      return true;
    }

    mFind = do_CreateInstance("@mozilla.org/embedcomp/rangefind;1");
    if (!mFind) {
      return false;
    }

    mFind->SetCaseSensitive(mCaseSensitive);
    mFind->SetWordBreaker(nullptr);

    return true;
  }

  nsCOMPtr<nsIWebBrowserFind> mWebBrowserFind;

  
  nsWeakPtr mDocShell;
  nsWeakPtr mPresShell;
  nsWeakPtr mSelectionController;
                                          
};
