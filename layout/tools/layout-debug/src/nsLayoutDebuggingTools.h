





#ifndef nsLayoutDebuggingTools_h
#define nsLayoutDebuggingTools_h

#include "nsILayoutDebuggingTools.h"
#include "nsIDocShell.h"
#include "nsCOMPtr.h"

class nsLayoutDebuggingTools : public nsILayoutDebuggingTools {

public:
    nsLayoutDebuggingTools();

    NS_DECL_ISUPPORTS

    NS_DECL_NSILAYOUTDEBUGGINGTOOLS

protected:
    virtual ~nsLayoutDebuggingTools();

    void ForceRefresh();
    nsresult GetBoolPref(const char * aPrefName, bool *aValue);
    nsresult SetBoolPrefAndRefresh(const char * aPrefName, bool aNewValue);

    nsCOMPtr<nsIDocShell> mDocShell;

    bool mEditorMode;
    bool mVisualDebugging;
    bool mVisualEventDebugging;
    bool mPaintFlashing;
    bool mPaintDumping;
    bool mInvalidateDumping;
    bool mEventDumping;
    bool mMotionEventDumping;
    bool mCrossingEventDumping;
    bool mReflowCounts;
};

#endif
