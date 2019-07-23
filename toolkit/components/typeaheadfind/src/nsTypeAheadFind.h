







































#include "nsISelectionController.h"
#include "nsIController.h"
#include "nsIControllers.h"
#include "nsIObserver.h"
#include "nsUnicharUtils.h"
#include "nsIFind.h"
#include "nsIWebBrowserFind.h"
#include "nsWeakReference.h"
#include "nsIPresShell.h"
#include "nsPresContext.h"
#include "nsISelection.h"
#include "nsIDOMRange.h"
#include "nsIDocShellTreeItem.h"
#include "nsITypeAheadFind.h"
#include "nsISupportsArray.h"
#include "nsISound.h"

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
                             PRBool *aIsInsideLink, PRBool *aIsStartingLink);

  void GetSelection(nsIPresShell *aPresShell, nsISelectionController **aSelCon, 
                    nsISelection **aDomSel);
  PRBool IsRangeVisible(nsIPresShell *aPresShell, nsPresContext *aPresContext,
                        nsIDOMRange *aRange, PRBool aMustBeVisible, 
                        PRBool aGetTopVisibleLeaf, nsIDOMRange **aNewRange,
                        PRBool *aUsesIndependentSelection);
  nsresult FindItNow(nsIPresShell *aPresShell, PRBool aIsLinksOnly,
                     PRBool aIsFirstVisiblePreferred, PRBool aFindPrev,
                     PRUint16* aResult);
  nsresult GetSearchContainers(nsISupports *aContainer,
                               nsISelectionController *aSelectionController,
                               PRBool aIsFirstVisiblePreferred,
                               PRBool aFindPrev, nsIPresShell **aPresShell,
                               nsPresContext **aPresContext);

  
  
  NS_HIDDEN_(already_AddRefed<nsIPresShell>) GetPresShell();

  
  nsString mTypeAheadBuffer;
  nsCString mNotFoundSoundURL;

  
  
  
  PRBool mLinksOnlyPref;
  PRBool mStartLinksOnlyPref;
  PRPackedBool mLinksOnly;
  PRBool mCaretBrowsingOn;
  nsCOMPtr<nsIDOMElement> mFoundLink;     
  nsCOMPtr<nsIDOMElement> mFoundEditable; 
  nsCOMPtr<nsIDOMWindow> mCurrentWindow;
  
  
  PRUint32 mLastFindLength;

  
  
  nsCOMPtr<nsISound> mSoundInterface;
  PRBool mIsSoundInitialized;
  
  
  nsCOMPtr<nsIDOMRange> mStartFindRange;
  nsCOMPtr<nsIDOMRange> mSearchRange;
  nsCOMPtr<nsIDOMRange> mStartPointRange;
  nsCOMPtr<nsIDOMRange> mEndPointRange;

  
  nsCOMPtr<nsIFind> mFind;
  nsCOMPtr<nsIWebBrowserFind> mWebBrowserFind;

  
  nsWeakPtr mDocShell;
  nsWeakPtr mPresShell;
  nsWeakPtr mSelectionController;
                                          
};
