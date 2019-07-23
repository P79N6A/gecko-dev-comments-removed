





































#include "nsILayoutDebuggingTools.h"
#include "nsIDocShell.h"
#include "nsCOMPtr.h"
#include "nsIPref.h"

class nsLayoutDebuggingTools : public nsILayoutDebuggingTools {

public:
    nsLayoutDebuggingTools();
    virtual ~nsLayoutDebuggingTools();

    NS_DECL_ISUPPORTS

    NS_DECL_NSILAYOUTDEBUGGINGTOOLS

protected:
    void ForceRefresh();
    nsresult GetBoolPref(const char * aPrefName, PRBool *aValue);
    nsresult SetBoolPrefAndRefresh(const char * aPrefName, PRBool aNewValue);

    nsCOMPtr<nsIDocShell> mDocShell;
    nsCOMPtr<nsIPref> mPrefs;

    PRBool mEditorMode;
    PRBool mVisualDebugging;
    PRBool mVisualEventDebugging;
    PRBool mPaintFlashing;
    PRBool mPaintDumping;
    PRBool mInvalidateDumping;
    PRBool mEventDumping;
    PRBool mMotionEventDumping;
    PRBool mCrossingEventDumping;
    PRBool mReflowCounts;
};
