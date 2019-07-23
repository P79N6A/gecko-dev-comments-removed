





































#ifndef nsMathMLParts_h___
#define nsMathMLParts_h___

#include "nscore.h"
#include "nsISupports.h"


nsIFrame* NS_NewMathMLForeignFrameWrapper(nsIPresShell* aPresShell, nsStyleContext* aContext);
nsIFrame* NS_NewMathMLTokenFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);
nsIFrame* NS_NewMathMLmoFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);
nsIFrame* NS_NewMathMLmrowFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);
nsIFrame* NS_NewMathMLmphantomFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);
nsIFrame* NS_NewMathMLmpaddedFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);
nsIFrame* NS_NewMathMLmspaceFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);
nsIFrame* NS_NewMathMLmsFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);
nsIFrame* NS_NewMathMLmfencedFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);
nsIFrame* NS_NewMathMLmfracFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);
nsIFrame* NS_NewMathMLmsubFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);
nsIFrame* NS_NewMathMLmsupFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);
nsIFrame* NS_NewMathMLmsubsupFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);
nsIFrame* NS_NewMathMLmunderFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);
nsIFrame* NS_NewMathMLmoverFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);
nsIFrame* NS_NewMathMLmunderoverFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);
nsIFrame* NS_NewMathMLmmultiscriptsFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);
nsIFrame* NS_NewMathMLmstyleFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);
nsIFrame* NS_NewMathMLmtableOuterFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);
nsIFrame* NS_NewMathMLmtableFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);
nsIFrame* NS_NewMathMLmtrFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);
nsIFrame* NS_NewMathMLmtdFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);
nsIFrame* NS_NewMathMLmtdInnerFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);
nsIFrame* NS_NewMathMLmsqrtFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);
nsIFrame* NS_NewMathMLmrootFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);
nsIFrame* NS_NewMathMLmactionFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);
nsIFrame* NS_NewMathMLmencloseFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);

nsIFrame* NS_NewMathMLmathBlockFrame(nsIPresShell* aPresShell, nsStyleContext* aContext, PRUint32 aFlags);
nsIFrame* NS_NewMathMLmathInlineFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);
inline nsIFrame* NS_CreateNewMathMLmathBlockFrame(nsIPresShell* aPresShell, nsStyleContext* aContext)
{
  return NS_NewMathMLmathBlockFrame(aPresShell, aContext, 0);
}
#endif 
