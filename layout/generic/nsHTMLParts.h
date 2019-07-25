






































#ifndef nsHTMLParts_h___
#define nsHTMLParts_h___

#include "nscore.h"
#include "nsISupports.h"
#include "nsIFrame.h"
class nsIAtom;
class nsNodeInfoManager;
class nsIContent;
class nsIContentIterator;
class nsIDocument;
class nsIFrame;
class nsIHTMLContentSink;
class nsIFragmentContentSink;
class nsStyleContext;
class nsIURI;
class nsString;
class nsIPresShell;
class nsIChannel;
class nsTableColFrame;

















#define NS_BLOCK_MARGIN_ROOT              NS_FRAME_STATE_BIT(22)
#define NS_BLOCK_FLOAT_MGR                NS_FRAME_STATE_BIT(23)
#define NS_BLOCK_CLIP_PAGINATED_OVERFLOW  NS_FRAME_STATE_BIT(28)
#define NS_BLOCK_HAS_FIRST_LETTER_STYLE   NS_FRAME_STATE_BIT(29)
#define NS_BLOCK_FRAME_HAS_OUTSIDE_BULLET NS_FRAME_STATE_BIT(30)
#define NS_BLOCK_HAS_FIRST_LETTER_CHILD   NS_FRAME_STATE_BIT(31)


#define NS_BLOCK_FLAGS_MASK               (NS_BLOCK_MARGIN_ROOT | \
                                           NS_BLOCK_FLOAT_MGR | \
                                           NS_BLOCK_CLIP_PAGINATED_OVERFLOW | \
                                           NS_BLOCK_HAS_FIRST_LETTER_STYLE | \
                                           NS_BLOCK_FRAME_HAS_OUTSIDE_BULLET | \
                                           NS_BLOCK_HAS_FIRST_LETTER_CHILD)




nsIFrame*
NS_NewBlockFrame(nsIPresShell* aPresShell, nsStyleContext* aContext, PRUint32 aFlags = 0);



nsresult
NS_NewAttributeContent(nsNodeInfoManager *aNodeInfoManager,
                       PRInt32 aNameSpaceID, nsIAtom* aAttrName,
                       nsIContent** aResult);





nsIFrame*
NS_NewSelectsAreaFrame(nsIPresShell* aPresShell, nsStyleContext* aContext, PRUint32 aFlags);


inline nsIFrame* NS_NewBlockFormattingContext(nsIPresShell* aPresShell,
                                              nsStyleContext* aStyleContext)
{
  return NS_NewBlockFrame(aPresShell, aStyleContext,
                          NS_BLOCK_FLOAT_MGR | NS_BLOCK_MARGIN_ROOT);
}

nsIFrame*
NS_NewBRFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);

nsIFrame*
NS_NewCommentFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);


nsIFrame*
NS_NewSubDocumentFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);

nsIFrame*
NS_NewHTMLFramesetFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);

nsIFrame*
NS_NewViewportFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);
nsIFrame*
NS_NewCanvasFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);
nsIFrame*
NS_NewImageFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);
nsIFrame*
NS_NewInlineFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);
nsIFrame*
NS_NewObjectFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);
nsIFrame*
NS_NewTextFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);
nsIFrame*
NS_NewContinuingTextFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);
nsIFrame*
NS_NewEmptyFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);
inline nsIFrame*
NS_NewWBRFrame(nsIPresShell* aPresShell, nsStyleContext* aContext) {
  return NS_NewEmptyFrame(aPresShell, aContext);
}

nsIFrame*
NS_NewColumnSetFrame(nsIPresShell* aPresShell, nsStyleContext* aContext, PRUint32 aStateFlags);

nsIFrame*
NS_NewSimplePageSequenceFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);
nsIFrame*
NS_NewPageFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);
nsIFrame*
NS_NewPageContentFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);
nsIFrame*
NS_NewPageBreakFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);
nsIFrame*
NS_NewFirstLetterFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);
nsIFrame*
NS_NewFirstLineFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);


nsIFrame*
NS_NewGfxButtonControlFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);
nsIFrame*
NS_NewNativeButtonControlFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);
nsIFrame*
NS_NewImageControlFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);
nsIFrame*
NS_NewHTMLButtonControlFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);
nsIFrame*
NS_NewGfxCheckboxControlFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);
nsIFrame*
NS_NewNativeCheckboxControlFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);
nsIFrame*
NS_NewFieldSetFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);
nsIFrame*
NS_NewFileControlFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);
nsIFrame*
NS_NewLegendFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);
nsIFrame*
NS_NewNativeTextControlFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);
nsIFrame*
NS_NewTextControlFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);
nsIFrame*
NS_NewGfxAutoTextControlFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);
nsIFrame*
NS_NewGfxRadioControlFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);
nsIFrame*
NS_NewNativeRadioControlFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);
nsIFrame*
NS_NewNativeSelectControlFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);
nsIFrame*
NS_NewListControlFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);
nsIFrame*
NS_NewComboboxControlFrame(nsIPresShell* aPresShell, nsStyleContext* aContext, PRUint32 aFlags);
nsIFrame*
NS_NewIsIndexFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);


nsIFrame*
NS_NewTableOuterFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);
nsIFrame*
NS_NewTableFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);
nsIFrame*
NS_NewTableCaptionFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);
nsTableColFrame*
NS_NewTableColFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);
nsIFrame*
NS_NewTableColGroupFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);
nsIFrame*
NS_NewTableRowFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);
nsIFrame*
NS_NewTableRowGroupFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);
nsIFrame*
NS_NewTableCellFrame(nsIPresShell* aPresShell, nsStyleContext* aContext, PRBool aIsBorderCollapse);

nsresult
NS_NewHTMLContentSink(nsIHTMLContentSink** aInstancePtrResult,
                      nsIDocument* aDoc, nsIURI* aURL,
                      nsISupports* aContainer, 
                      nsIChannel* aChannel);
nsresult
NS_NewHTMLFragmentContentSink(nsIFragmentContentSink** aInstancePtrResult);
nsresult
NS_NewHTMLFragmentContentSink2(nsIFragmentContentSink** aInstancePtrResult);



nsresult
NS_NewHTMLParanoidFragmentSink(nsIFragmentContentSink** aInstancePtrResult);
nsresult
NS_NewHTMLParanoidFragmentSink2(nsIFragmentContentSink** aInstancePtrResult);
void
NS_HTMLParanoidFragmentSinkShutdown();
#endif 
