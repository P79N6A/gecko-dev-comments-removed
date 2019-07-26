




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
  virtual ~nsTypeAheadFind();

  NS_DECL_ISUPPORTS
  NS_DECL_NSITYPEAHEADFIND
  NS_DECL_NSIOBSERVER

protected:
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
                     PRUint16* aResult);
  nsresult GetSearchContainers(nsISupports *aContainer,
                               nsISelectionController *aSelectionController,
                               bool aIsFirstVisiblePreferred,
                               bool aFindPrev, nsIPresShell **aPresShell,
                               nsPresContext **aPresContext);

  
  
  NS_HIDDEN_(already_AddRefed<nsIPresShell>) GetPresShell();

  
  nsString mTypeAheadBuffer;
  nsCString mNotFoundSoundURL;

  
  
  bool mStartLinksOnlyPref;
  bool mCaretBrowsingOn;
  nsCOMPtr<nsIDOMElement> mFoundLink;     
  nsCOMPtr<nsIDOMElement> mFoundEditable; 
  nsCOMPtr<nsIDOMWindow> mCurrentWindow;
  
  
  PRUint32 mLastFindLength;

  
  
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
