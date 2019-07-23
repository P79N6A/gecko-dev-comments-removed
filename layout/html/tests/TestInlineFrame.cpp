



































#include <stdio.h>
#include "nscore.h"
#include "nsCRT.h"
#include "nsHTMLParts.h"
#include "nsIDocument.h"
#include "nsDocument.h"
#include "nsHTMLTagContent.h"
#include "nsCoord.h"
#include "nsSplittableFrame.h"
#include "nsIContentDelegate.h"
#include "nsPresContext.h"
#include "nsInlineFrame.h"
#include "nsIAtom.h"
#include "nsAutoPtr.h"
#include "nsStyleSet.h"




class MyDocument : public nsDocument {
public:
  MyDocument();
  NS_IMETHOD StartDocumentLoad(const char* aCommand,
                               nsIChannel* aChannel,
                               nsILoadGroup* aLoadGroup,
                               nsISupports* aContainer,
                               nsIStreamListener **aDocListener)
  {
    return NS_OK;
  }

protected:
  virtual ~MyDocument();
};

MyDocument::MyDocument()
{
}

MyDocument::~MyDocument()
{
}






class FixedSizeFrame : public nsSplittableFrame {
public:
  FixedSizeFrame(nsIContent* aContent,
                 nsIFrame* aParentFrame);

  NS_IMETHOD Reflow(nsPresContext* aPresContext,
                    nsReflowMetrics& aDesiredSize,
                    const nsReflowState& aReflowState,
                    nsReflowStatus& aStatus);

  PRBool       IsSplittable() const;
};

class FixedSizeContent : public nsHTMLTagContent {
public:
  FixedSizeContent(nscoord aWidth,
                   nscoord aHeight,
                   PRBool  aIsSplittable = PR_FALSE);

  
  nscoord   GetWidth() {return mWidth;}
  void      SetWidth(nscoord aWidth);
  nscoord   GetHeight() {return mHeight;}
  void      SetHeight(nscoord aHeight);

  void      ToHTML(nsString& out);
  PRBool    IsSplittable() const {return mIsSplittable;}
  void      SetIsSplittable(PRBool aIsSplittable) {mIsSplittable = aIsSplittable;}


private:
  nscoord mWidth, mHeight;
  PRBool  mIsSplittable;
};




FixedSizeFrame::FixedSizeFrame(nsIContent* aContent,
                               nsIFrame* aParentFrame)
  : nsSplittableFrame(aContent, aParentFrame)
{
}

NS_METHOD FixedSizeFrame::Reflow(nsPresContext*      aPresContext,
                                 nsReflowMetrics&     aDesiredSize,
                                 const nsReflowState& aReflowState,
                                 nsReflowStatus&      aStatus)
{
  NS_PRECONDITION((aReflowState.availableWidth > 0) && (aReflowState.availableHeight > 0),
                  "bad max size");
  FixedSizeContent* content = (FixedSizeContent*)mContent;
  nsReflowStatus    status = NS_FRAME_COMPLETE;
  FixedSizeFrame*   prevInFlow = (FixedSizeFrame*)mPrevInFlow;

  aDesiredSize.width = content->GetWidth();
  aDesiredSize.height = content->GetHeight();

  
  if (nsnull != prevInFlow) {
    aDesiredSize.width -= prevInFlow->mRect.width;
  } else if ((aDesiredSize.width > aReflowState.maxSize.width) && content->IsSplittable()) {
    aDesiredSize.width = aReflowState.maxSize.width;
    status = NS_FRAME_NOT_COMPLETE;
  }

  aDesiredSize.ascent = aDesiredSize.height;
  aDesiredSize.descent = 0;
  if (nsnull != aDesiredSize.maxElementSize) {
    aDesiredSize.maxElementSize->width = aDesiredSize.width;
    aDesiredSize.maxElementSize->height = aDesiredSize.height;
  }

  return status;
}

PRBool FixedSizeFrame::IsSplittable() const
{
  FixedSizeContent* content = (FixedSizeContent*)mContent;

  return content->IsSplittable();
}




FixedSizeContent::FixedSizeContent(nscoord aWidth,
                                   nscoord aHeight,
                                   PRBool  aIsSplittable)
  : nsHTMLTagContent()
{
  mWidth = aWidth;
  mHeight = aHeight;
  mIsSplittable = aIsSplittable;
}


void FixedSizeContent::SetWidth(nscoord aWidth)
{
  NS_NOTYETIMPLEMENTED("set width");
}


void FixedSizeContent::SetHeight(nscoord aHeight)
{
  NS_NOTYETIMPLEMENTED("set height");
}

void FixedSizeContent::ToHTML(nsString& out)
{
}




class InlineFrame : public nsInlineFrame
{
public:
  InlineFrame(nsIContent* aContent,
              nsIFrame* aParent);

  
  nsIFrame* OverflowList() {return mOverflowList;}
  void      ClearOverflowList() {mOverflowList = nsnull;}
  PRBool    LastContentIsComplete() {return mLastContentIsComplete;}

  
  PRInt32   MaxChildWidth();
  PRInt32   MaxChildHeight();
};

InlineFrame::InlineFrame(nsIContent* aContent,
                         nsIFrame*   aParent)
  : nsInlineFrame(aContent, aParent)
{
}

#if 0
PRInt32 InlineFrame::MaxChildWidth()
{
  PRInt32 maxWidth = 0;

  nsIFrame* f;
  for (f = GetFirstChild(); nsnull != f; f->GetNextSibling(f)) {
    if (f->GetWidth() > maxWidth) {
      maxWidth = f->GetWidth();
    }
  }

  return maxWidth;
}

PRInt32 InlineFrame::MaxChildHeight()
{
  PRInt32 maxHeight = 0;

  for (nsIFrame* f = GetFirstChild(); nsnull != f; f = f->GetNextSibling()) {
    if (f->GetHeight() > maxHeight) {
      maxHeight = f->GetHeight();
    }
  }

  return maxHeight;
}





static PRInt32
LengthOf(nsIFrame* aChildList)
{
  PRInt32 result = 0;

  while (nsnull != aChildList) {
    aChildList = aChildList->GetNextSibling();
    result++;
  }

  return result;
}












static PRBool
TestReflowUnmapped(nsPresContext* presContext)
{
  
  nsIContent* b;
  NS_NewHTMLContainer(&b, NS_NewAtom("span"));

  
  b->AppendChildTo(new FixedSizeContent(100, 100));
  b->AppendChildTo(new FixedSizeContent(300, 300));
  b->AppendChildTo(new FixedSizeContent(200, 200));

  
  
  InlineFrame*      f = new InlineFrame(b, 0, nsnull);
  nsRefPtr<nsStyleContext> styleContext;
  styleContext = presContext->StyleSet()->ResolveStyleFor(b, nsnull);

  f->SetStyleContext(styleContext);

  
  nsReflowMetrics   reflowMetrics;
  nsSize            maxSize(1000, 1000);
  nsReflowStatus    status;

  status = f->ResizeReflow(presContext, reflowMetrics, maxSize, nsnull);

  
  if (NS_FRAME_IS_NOT_COMPLETE(status)) {
    printf("ReflowUnmapped: initial reflow not complete: %d\n", status);
    return PR_FALSE;
  }

  
  if (f->ChildCount() != b->ChildCount()) {
    printf("ReflowUnmapped: wrong number of child frames: %d\n", f->ChildCount());
    return PR_FALSE;
  }

  
  if (f->GetLastContentOffset() != (b->ChildCount() - 1)) {
    printf("ReflowUnmapped: wrong last content offset: %d\n", f->GetLastContentOffset());
    return PR_FALSE;
  }

  
  for (PRInt32 i = 0; i < b->ChildCount(); i++) {
    FixedSizeContent* childContent = (FixedSizeContent*)b->ChildAt(i);
    nsIFrame*         childFrame = f->ChildAt(i);

    if ((childFrame->GetWidth() != childContent->GetWidth()) ||
        (childFrame->GetHeight() != childContent->GetHeight())) {
      printf("ReflowUnmapped: child frame size incorrect: %d\n", i);
      return PR_FALSE;
    }
  }

  
  if (reflowMetrics.height != f->MaxChildHeight()) {
    printf("ReflowUnmapped: wrong frame height: %d\n", reflowMetrics.height);
    return PR_FALSE;
  }

  
  nscoord x = 0;
  for (nsIFrame* child = f->FirstChild(); child; child = child->GetNextSibling()) {
    nsPoint origin;

    child->GetOrigin(origin);
    if (origin.x != x) {
      printf("ReflowUnmapped: child x-origin is incorrect: %d\n", x);
      return PR_FALSE;
    }

    x += child->GetWidth();
  }

  
  if (reflowMetrics.width != x) {
    printf("ReflowUnmapped: wrong frame width: %d\n", reflowMetrics.width);
    return PR_FALSE;
  }

  NS_RELEASE(b);
  return PR_TRUE;
}










static PRBool
TestChildrenThatDontFit(nsPresContext* presContext)
{
  
  nsIContent* b;
  NS_NewHTMLContainer(&b, NS_NewAtom("span"));

  
  b->AppendChildTo(new FixedSizeContent(100, 100));

  
  
  InlineFrame*      f = new InlineFrame(b, 0, nsnull);
  nsRefPtr<nsStyleContext> styleContext;
  styleContext = presContext->StyleSet()->ResolveStyleFor(b, nsnull);

  f->SetStyleContext(styleContext);

  
  

  
  
  nsReflowMetrics   reflowMetrics;
  nsSize            maxSize(10, 1000);
  nsReflowStatus    status;

  status = f->ResizeReflow(presContext, reflowMetrics, maxSize, nsnull);

  
  if (NS_FRAME_IS_NOT_COMPLETE(status)) {
    printf("ChildrenThatDontFIt: reflow unmapped isn't complete (#1a): %d\n", status);
    return PR_FALSE;
  }

  
  if (reflowMetrics.width != f->FirstChild()->GetWidth()) {
    printf("ChildrenThatDontFit: frame width is wrong (#1a): %d\n", f->FirstChild()->GetWidth());
    return PR_FALSE;
  }

  
  

  
  b->AppendChildTo(new FixedSizeContent(300, 300));
  b->AppendChildTo(new FixedSizeContent(200, 200));

  
  InlineFrame*  f1 = new InlineFrame(b, 0, nsnull);
  f1->SetStyleContext(styleContext);

  
  
  maxSize.SizeTo(10, 1000);
  status = f1->ResizeReflow(presContext, reflowMetrics, maxSize, nsnull);

  
  if (NS_FRAME_IS_NOT_COMPLETE(status)) {
    printf("ChildrenThatDontFIt: reflow unmapped isn't not complete (#1b): %d\n", status);
    return PR_FALSE;
  }

  
  if (reflowMetrics.width != f1->FirstChild()->GetWidth()) {
    printf("ChildrenThatDontFit: frame width is wrong (#1b): %d\n", f1->FirstChild()->GetWidth());
    return PR_FALSE;
  }

  
  if (f1->ChildCount() != 1) {
    printf("ChildrenThatDontFit: more than one child frame (#1b): %d\n", f1->ChildCount());
    return PR_FALSE;
  }

  
  
  
  if (nsnull != f1->OverflowList()) {
    printf("ChildrenThatDontFit: unexpected overflow list (#1b)\n");
    return PR_FALSE;
  }

  
  

  
  
  
  maxSize.width = 1000;
  maxSize.height = 10;
  reflowMetrics.height = 0;
  status = f1->ResizeReflow(presContext, reflowMetrics, maxSize, nsnull);

  
  if (NS_FRAME_IS_NOT_COMPLETE(status)) {
    printf("ChildrenThatDontFit: resize isn't complete (#2): %d\n", status);
    return PR_FALSE;
  }

  
  if (f1->ChildCount() != b->ChildCount()) {
    printf("ChildrenThatDontFit: wrong child count (#2): %d\n", f1->ChildCount());
    return PR_FALSE;
  }

  
  if (reflowMetrics.height != f1->MaxChildHeight()) {
    printf("ChildrenThatDontFit: height is wrong (#2): %d\n", reflowMetrics.height);
    return PR_FALSE;
  }

  
  

  
  nscoord oldHeight = reflowMetrics.height;
  maxSize.height = 10;
  reflowMetrics.height = 0;
  status = f1->ResizeReflow(presContext, reflowMetrics, maxSize, nsnull);

  
  if (NS_FRAME_IS_NOT_COMPLETE(status) || (f1->ChildCount() != b->ChildCount())) {
    printf("ChildrenThatDontFit: reflow mapped failed (#3)\n");
    return PR_FALSE;
  }

  
  if (reflowMetrics.height != oldHeight) {
    printf("ChildrenThatDontFit: height is wrong (#3): %d\n", reflowMetrics.height);
    return PR_FALSE;
  }

  
  

  
  
  maxSize.width = 10;
  maxSize.height = 1000;
  status = f1->ResizeReflow(presContext, reflowMetrics, maxSize, nsnull);

  
  if (NS_FRAME_IS_NOT_COMPLETE(status)) {
    printf("ChildrenThatDontFIt: reflow mapped isn't not complete (#4): %d\n", status);
    return PR_FALSE;
  }

  
  if (f1->ChildCount() > 1) {
    printf("ChildrenThatDontFIt: reflow mapped too many children (#4): %d\n", f1->ChildCount());
    return PR_FALSE;
  }

  
  if (reflowMetrics.width != f1->FirstChild()->GetWidth()) {
    printf("ChildrenThatDontFit: frame width is wrong (#4): %d\n", f1->FirstChild()->GetWidth());
    return PR_FALSE;
  }

  NS_RELEASE(b);
  return PR_TRUE;
}








static PRBool
TestOverflow(nsPresContext* presContext)
{
  
  nsIContent* b;
  NS_NewHTMLContainer(&b, NS_NewAtom("span"));

  
  b->AppendChildTo(new FixedSizeContent(100, 100));
  b->AppendChildTo(new FixedSizeContent(300, 300));
  b->AppendChildTo(new FixedSizeContent(200, 200));

  
  
  InlineFrame*      f = new InlineFrame(b, 0, nsnull);
  nsRefPtr<nsStyleContext> styleContext;
  styleContext = presContext->StyleSet()->ResolveStyleFor(b, nsnull);

  f->SetStyleContext(styleContext);

  
  

  
  nsReflowMetrics   reflowMetrics;
  nsSize            maxSize(150, 1000);
  nsReflowStatus    status;

  status = f->ResizeReflow(presContext, reflowMetrics, maxSize, nsnull);
  NS_ASSERTION(nsIFrame::frNotComplete == status, "bad status");
  NS_ASSERTION(f->ChildCount() == 1, "bad child count");

  
  if ((nsnull == f->OverflowList()) || (LengthOf(f->OverflowList()) != 1)) {
    printf("Overflow: no overflow list (#1)\n");
    return PR_FALSE;
  }

  
  

  
  
  maxSize.width = 1000;
  status = f->ResizeReflow(presContext, reflowMetrics, maxSize, nsnull);
  NS_ASSERTION(nsIFrame::frComplete == status, "isn't complete");

  
  if (nsnull != f->OverflowList()) {
    printf("Overflow: overflow list should be empty (#2)\n");
    return PR_FALSE;
  }

  
  

  
  
  
  maxSize.width = f->FirstChild()->GetWidth();
  status = f->ResizeReflow(presContext, reflowMetrics, maxSize, nsnull);
  NS_ASSERTION((f->ChildCount() == 1) && (LengthOf(f->OverflowList()) == 2), "bad overflow list");

  
  InlineFrame* f1 = new InlineFrame(b, 0, nsnull);

  f1->SetStyleContext(f->GetStyleContext(presContext));
  f->SetNextSibling(f1);
  f->SetNextInFlow(f1);
  f1->SetPrevInFlow(f);

  
  maxSize.width = 1000;
  status = f1->ResizeReflow(presContext, reflowMetrics, maxSize, nsnull);
  NS_ASSERTION(NS_FRAME_IS_COMPLETE(status), "bad continuing frame");

  
  if (nsnull != f->OverflowList()) {
    printf("Overflow: overflow list should be empty (#3)\n");
    return PR_FALSE;
  }

  
  if (f1->ChildCount() != (b->ChildCount() - f->ChildCount())) {
    printf("Overflow: continuing frame child count is wrong (#3): %d\n", f1->ChildCount());
    return PR_FALSE;
  }

  
  if (f1->GetFirstContentOffset() != f1->FirstChild()->GetIndexInParent()) {
    printf("Overflow: continuing frame bad first content offset (#3): %d\n", f1->GetFirstContentOffset());
    return PR_FALSE;
  }
  if (f1->GetLastContentOffset() != (f1->LastChild()->GetIndexInParent())) {
    printf("Overflow: continuing frame bad last content offset (#3): %d\n", f1->GetLastContentOffset());
    return PR_FALSE;
  }

  NS_RELEASE(b);
  return PR_TRUE;
}












static PRBool
TestPushingPulling(nsPresContext* presContext)
{
  
  nsIContent* b;
  NS_NewHTMLContainer(&b, NS_NewAtom("span"));

  
  b->AppendChildTo(new FixedSizeContent(100, 100));
  b->AppendChildTo(new FixedSizeContent(300, 300));
  b->AppendChildTo(new FixedSizeContent(200, 200));

  
  
  InlineFrame*      f = new InlineFrame(b, 0, nsnull);
  nsRefPtr<nsStyleContext> styleContext;
  styleContext = presContext->StyleSet()->ResolveStyleFor(b, nsnull);

  f->SetStyleContext(styleContext);

  
  nsReflowMetrics   reflowMetrics;
  nsSize            maxSize(100, 1000);
  nsReflowStatus    status;

  status = f->ResizeReflow(presContext, reflowMetrics, maxSize, nsnull);
  NS_ASSERTION(NS_FRAME_IS_NOT_COMPLETE(status), "bad status");
  NS_ASSERTION(f->ChildCount() == 1, "bad child count");

  
  

  
  
  
  
  NS_ASSERTION(nsnull == f->OverflowList(), "unexpected overflow list");
  InlineFrame* f1 = new InlineFrame(b, 0, nsnull);

  f1->SetStyleContext(f->GetStyleContext(presContext));
  f->SetNextSibling(f1);
  f->SetNextInFlow(f1);
  f1->SetPrevInFlow(f);

  
  
  maxSize.width = 1000;
  status = f1->ResizeReflow(presContext, reflowMetrics, maxSize, nsnull);
  NS_ASSERTION(NS_FRAME_IS_COMPLETE(status), "bad continuing frame");

  
  if (f1->ChildCount() != (b->ChildCount() - f->ChildCount())) {
    printf("PushPull: continuing frame child count is wrong (#1): %d\n", f1->ChildCount());
    return PR_FALSE;
  }

  
  

  
  
  maxSize.width = 400;
  status = f->ResizeReflow(presContext, reflowMetrics, maxSize, nsnull);
  NS_ASSERTION(nsIFrame::frNotComplete == status, "bad status");
  NS_ASSERTION(f->ChildCount() == 2, "bad child count");
  NS_ASSERTION(f1->ChildCount() == 1, "bad continuing child count");

  
  if (f->GetLastContentOffset() != (f->ChildCount() - 1)) {
    printf("PushPull: bad last content offset (#2): %d\n", f->GetLastContentOffset());
    return PR_FALSE;
  }

  
  
  if (f1->GetFirstContentOffset() != f1->FirstChild()->GetIndexInParent()) {
    printf("PushPull: continuing frame bad first content offset (#2): %d\n", f1->GetFirstContentOffset());
    return PR_FALSE;
  }
  if (f1->GetLastContentOffset() != (f1->LastChild()->GetIndexInParent())) {
    printf("PushPull: continuing frame bad last content offset (#2): %d\n", f1->GetLastContentOffset());
    return PR_FALSE;
  }

  
  

  
  
  
  
  
  maxSize.width = f->FirstChild()->GetWidth();
  status = f->ResizeReflow(presContext, reflowMetrics, maxSize, nsnull);
  NS_ASSERTION(NS_FRAME_IS_NOT_COMPLETE(status), "bad status");
  NS_ASSERTION(f->ChildCount() == 1, "bad child count");

  
  if (f->GetLastContentOffset() != 0) {
    printf("PushPull: bad last content offset after push (#3): %d\n",
           f->GetLastContentOffset());
    return PR_FALSE;
  }

  
  if (f1->ChildCount() != 2) {
    printf("PushPull: continuing child bad child count after push (#3): %d\n",
           f1->ChildCount());
    return PR_FALSE;
  }

  
  if (f1->GetFirstContentOffset() != f1->FirstChild()->GetIndexInParent()) {
    printf("PushPull: continuing child bad first content offset after push (#3): %d\n",
           f1->GetFirstContentOffset());
    return PR_FALSE;
  }

  
  if (f1->GetLastContentOffset() != (f1->LastChild()->GetIndexInParent())) {
    printf("PushPull: continuing child bad last content offset after push (#3): %d\n",
           f1->GetLastContentOffset());
    return PR_FALSE;
  }

  
  maxSize.width = 1000;
  status = f->ResizeReflow(presContext, reflowMetrics, maxSize, nsnull);
  NS_ASSERTION(nsIFrame::frComplete == status, "bad status");

  
  if ((f->ChildCount() != b->ChildCount()) ||
      (f->GetLastContentOffset() != (f->ChildCount() - 1))) {
    printf("PushPull: failed to pull up all the child frames (#3)\n");
    return PR_FALSE;
  }

  
  if (f1->ChildCount() != 0) {
    printf("PushPull: continuing child is not empty after pulling up all children (#3)\n");
    return PR_FALSE;
  }

  
  

  
  
  
  
  maxSize.width = f->FirstChild()->GetWidth();
  f1->SetFirstContentOffset(f->NextChildOffset());  
  status = f->ResizeReflow(presContext, reflowMetrics, maxSize, nsnull);
  NS_ASSERTION(nsIFrame::frNotComplete == status, "bad status");
  NS_ASSERTION(f->ChildCount() == 1, "bad child count");

  
  if (f1->ChildCount() != 2) {
    printf("PushPull: continuing child bad child count after push (#4): %d\n",
           f1->ChildCount());
    return PR_FALSE;
  }

  
  if (f1->GetFirstContentOffset() != f1->FirstChild()->GetIndexInParent()) {
    printf("PushPull: continuing child bad first content offset after push (#4): %d\n",
           f1->GetFirstContentOffset());
    return PR_FALSE;
  }

  
  if (f1->GetLastContentOffset() != (f1->LastChild()->GetIndexInParent())) {
    printf("PushPull: continuing child bad last content offset after push (#4): %d\n",
           f1->GetLastContentOffset());
    return PR_FALSE;
  }

  
  

  
  
  maxSize.width = 300;
  status = f1->ResizeReflow(presContext, reflowMetrics, maxSize, nsnull);
  NS_ASSERTION(nsIFrame::frNotComplete == status, "bad status");
  NS_ASSERTION((f1->ChildCount() == 1) && (f1->GetLastContentOffset() == 1), "bad reflow");
  NS_ASSERTION(nsnull != f1->OverflowList(), "no overflow list");

  
  maxSize.width = 1000;
  status = f->ResizeReflow(presContext, reflowMetrics, maxSize, nsnull);
  NS_ASSERTION(nsIFrame::frComplete == status, "bad status");
  NS_ASSERTION((f->ChildCount() == 3) && (f->GetLastContentOffset() == 2), "bad mapping");

  
  if (f1->ChildCount() != 0) {
    printf("PushPull: continuing child isn't empty (#5)\n");
    return PR_FALSE;
  }
  if (nsnull != f1->OverflowList()) {
    printf("PushPull: continuing child still has overflow list (#5)\n");
    return PR_FALSE;
  }

  
  

  
  
  
  
  maxSize.width = 100;
  f1->BreakFromPrevFlow();
  status = f->ResizeReflow(presContext, reflowMetrics, maxSize, nsnull);
  NS_ASSERTION(nsIFrame::frNotComplete == status, "bad status");
  NS_ASSERTION((f->ChildCount() == 1) && (f->GetLastContentOffset() == 0), "bad mapping");

  maxSize.width = 300;
  f1->AppendToFlow(f);
  status = f1->ResizeReflow(presContext, reflowMetrics, maxSize, nsnull);
  NS_ASSERTION(nsIFrame::frNotComplete == status, "bad status");
  NS_ASSERTION((f1->ChildCount() == 1) && (f1->GetLastContentOffset() == 1), "bad mapping");
  NS_ASSERTION(nsnull != f1->OverflowList(), "no overflow list");

  
  InlineFrame* f2 = new InlineFrame(b, 0, nsnull);

  f2->SetStyleContext(f->GetStyleContext(presContext));
  f1->SetNextSibling(f2);
  f1->SetNextInFlow(f2);
  f2->SetPrevInFlow(f1);

  maxSize.width = 1000;
  status = f2->ResizeReflow(presContext, reflowMetrics, maxSize, nsnull);
  NS_ASSERTION(nsIFrame::frComplete == status, "bad status");
  NS_ASSERTION(nsnull == f1->OverflowList(), "overflow list");

  
  
  
  
  status = f->ResizeReflow(presContext, reflowMetrics, maxSize, nsnull);

  
  if (NS_FRAME_NOT_IS_COMPLETE(status)) {
    printf("PushPull: failed to pull-up across empty frame (#6)\n");
    return PR_FALSE;
  }

  
  if ((f->ChildCount() != 3) || (f->GetLastContentOffset() != 2)) {
    printf("PushPull: bad last content offset or child count (#6)\n");
    return PR_FALSE;
  }

  
  if (f1->ChildCount() != 0) {
    printf("PushPull: second frame isn't empty (#6)\n");
    return PR_FALSE;
  }
  if (f2->ChildCount() != 0) {
    printf("PushPull: third frame isn't empty (#6)\n");
    return PR_FALSE;
  }

  
  

  
  
  
  
  maxSize.width = 100;
  f1->BreakFromPrevFlow();
  f2->BreakFromPrevFlow();
  status = f->ResizeReflow(presContext, reflowMetrics, maxSize, nsnull);
  NS_ASSERTION(nsIFrame::frNotComplete == status, "bad status");
  NS_ASSERTION(f->ChildCount() == 1, "bad child count");
  NS_ASSERTION(f->GetLastContentOffset() == 0, "bad mapping");

  
  maxSize.width = 300;
  f1->AppendToFlow(f);
  status = f1->ResizeReflow(presContext, reflowMetrics, maxSize, nsnull);
  NS_ASSERTION(nsIFrame::frNotComplete == status, "bad status");
  NS_ASSERTION(f1->ChildCount() == 1, "bad child count");
  NS_ASSERTION((f1->GetFirstContentOffset() == 1) && (f1->GetLastContentOffset() == 1), "bad mapping");

  
  f2->AppendToFlow(f1);
  maxSize.width = 1000;
  status = f2->ResizeReflow(presContext, reflowMetrics, maxSize, nsnull);
  NS_ASSERTION(nsIFrame::frComplete == status, "bad status");
  NS_ASSERTION(f2->ChildCount() == 1, "bad child count");
  NS_ASSERTION((f2->GetFirstContentOffset() == 2) && (f2->GetLastContentOffset() == 2), "bad mapping");

  
  
  maxSize.width = 400;
  status = f->ResizeReflow(presContext, reflowMetrics, maxSize, nsnull);

  
  if (NS_FRAME_IS_COMPLETE(status)) {
    printf("PushPull: bad status (#7)\n");
    return PR_FALSE;
  }

  
  if ((f->ChildCount() != 2) || (f->GetLastContentOffset() != 1)) {
    printf("PushPull: bad last content offset or child count (#7)\n");
    return PR_FALSE;
  }

  
  if (f1->ChildCount() != 0) {
    printf("PushPull: second frame isn't empty (#7)\n");
    return PR_FALSE;
  }

  
  
  
  
  if (f1->GetFirstContentOffset() != f->NextChildOffset()) {
    printf("PushPull: second frame bad first content offset (#7): %d\n", f1->GetFirstContentOffset());
    return PR_FALSE;
  }

  
  if (f2->ChildCount() != 1) {
    printf("PushPull: third frame bad child count (#7): %d\n", f2->ChildCount());
    return PR_FALSE;
  }

  
  if ((f2->GetFirstContentOffset() != 2) || (f2->GetLastContentOffset() != 2)) {
    printf("PushPull: third frame bad content mapping (#7)\n");
    return PR_FALSE;
  }

  
  

  
  
  f2->BreakFromPrevFlow();
  f1->SetNextSibling(nsnull);

  maxSize.width = 1000;
  status = f->ResizeReflow(presContext, reflowMetrics, maxSize, nsnull);
  NS_ASSERTION(nsIFrame::frComplete == status, "bad status");
  NS_ASSERTION(f->ChildCount() == 3, "bad child count");
  f1->SetFirstContentOffset(f->NextChildOffset());  

  
  
  maxSize.width = 100;
  status = f->ResizeReflow(presContext, reflowMetrics, maxSize, nsnull);
  NS_ASSERTION(nsIFrame::frNotComplete == status, "bad status");
  NS_ASSERTION((f->ChildCount() == 1) && (f1->ChildCount() == 2), "bad child count");

  
  
  
  FixedSizeContent* c2 = (FixedSizeContent*)b->ChildAt(1);
  c2->SetIsSplittable(PR_TRUE);

  maxSize.width = 250;
  status = f->ResizeReflow(presContext, reflowMetrics, maxSize, nsnull);
  NS_ASSERTION(nsIFrame::frNotComplete == status, "bad status");

  
  if (f->ChildCount() != 2) {
    printf("PushPull: bad child count when pulling up (#8): %d\n", f->ChildCount());
    return PR_FALSE;
  }

  
  
  if ((f->GetLastContentOffset() != 1) || f->LastContentIsComplete()) {
    printf("PushPull: bad content mapping when pulling up (#8)\n");
    return PR_FALSE;
  }

  
  if (f1->ChildCount() != 2) {
    printf("PushPull: continuing frame bad child count (#8): %d\n", f1->ChildCount());
    return PR_FALSE;
  }

  
  if ((f1->GetFirstContentOffset() != f1->FirstChild()->GetIndexInParent()) ||
      (f1->GetLastContentOffset() != (f1->LastChild()->GetIndexInParent()))) {
    printf("PushPull: continuing frame bad mapping (#8)\n");
    return PR_FALSE;
  }

  
  if (f1->FirstChild()->GetPrevInFlow() != f->LastChild()) {
    printf("PushPull: continuing frame bad flow (#8)\n");
    return PR_FALSE;
  }

  NS_RELEASE(b);
  return PR_TRUE;
}
















static PRBool
TestSplittableChildren(nsPresContext* presContext)
{
  
  nsIContent* b;
  NS_NewHTMLContainer(&b, NS_NewAtom("span"));

  
  b->AppendChildTo(new FixedSizeContent(100, 100, PR_TRUE));
  b->AppendChildTo(new FixedSizeContent(300, 300, PR_TRUE));
  b->AppendChildTo(new FixedSizeContent(200, 200, PR_TRUE));

  
  
  InlineFrame*      f = new InlineFrame(b, 0, nsnull);
  nsRefPtr<nsStyleContext> styleContext;
  styleContext = presContext->StyleSet()->ResolveStyleFor(b, nsnull);

  f->SetStyleContext(styleContext);

  
  

  
  
  nsReflowMetrics         reflowMetrics;
  nsSize                  maxSize(50, 1000);
  nsIFrame::ReflowStatus  status;

  status = f->ResizeReflow(presContext, reflowMetrics, maxSize, nsnull);
  NS_ASSERTION(f->ChildCount() == 1, "bad child count");

  
  if (nsIFrame::frNotComplete != status) {
    printf("Splittable: inline frame should be incomplete (#1): %d\n", status);
    return PR_FALSE;
  }

  
  if (f->GetLastContentOffset() != 0) {
    printf("Splittable: wrong last content offset (#1): %d\n", f->GetLastContentOffset());
    return PR_FALSE;
  }

  
  if (f->LastContentIsComplete()) {
    printf("Splittable: child should not be complete (#1)\n");
    return PR_FALSE;
  }

  
  

  
  status = f->ResizeReflow(presContext, reflowMetrics, maxSize, nsnull);
  NS_ASSERTION(f->ChildCount() == 1, "bad child count");

  
  if ((nsIFrame::frNotComplete != status) ||
      (f->GetLastContentOffset() != 0) || (f->LastContentIsComplete())) {
    printf("Splittable: reflow mapped failed (#2)\n");
    return PR_FALSE;
  }

  
  if ((nsnull == f->OverflowList()) ||
      (f->OverflowList() != f->FirstChild()->GetNextInFlow())) {
    printf("Splittable: should be an overflow list (#2)\n");
    return PR_FALSE;
  }

  
  
  if (nsnull != f->FirstChild()->GetNextSibling()) {
    printf("Splittable: first child has overflow list as next sibling (#2)\n");
    return PR_FALSE;
  }

  
  

  
  
  status = f->ResizeReflow(presContext, reflowMetrics, maxSize, nsnull);
  NS_ASSERTION(nsIFrame::frNotComplete == status, "bad status");
  NS_ASSERTION(f->ChildCount() == 1, "bad child count");

  
  
  if ((f->GetLastContentOffset() != 0) || (f->LastContentIsComplete())) {
    printf("Splittable: reflow mapped again failed (#3)\n");
    return PR_FALSE;
  }

  
  if (LengthOf(f->OverflowList()) != 1) {
    printf("Splittable: reflow mapped again has bad overflow list (#3)\n");
    return PR_FALSE;
  }

  
  

  
  
  
  maxSize.width = 100;
  status = f->ResizeReflow(presContext, reflowMetrics, maxSize, nsnull);
  NS_ASSERTION(nsIFrame::frNotComplete == status, "bad status");
  NS_ASSERTION(f->ChildCount() == 1, "bad child count");

  
  if (f->GetLastContentOffset() != 0) {
    printf("Splittable: wrong last content offset (#4a): %d\n",
           f->GetLastContentOffset());
    return PR_FALSE;
  }

  
  if (!f->LastContentIsComplete()) {
    printf("Splittable: last child isn't complete (#4a)\n");
    return PR_FALSE;
  }

  
  if (nsnull != f->FirstChild()->GetNextInFlow()) {
    printf("Splittable: unexpected next-in-flow (#4a)\n");
    return PR_FALSE;
  }

  
  if (nsnull != f->OverflowList()) {
    printf("Splittable: unexpected overflow list (#4a)\n");
    return PR_FALSE;
  }

  
  

  
  
  maxSize.width = 50;
  status = f->ResizeReflow(presContext, reflowMetrics, maxSize, nsnull);
  NS_ASSERTION(!f->LastContentIsComplete(), "last child should be incomplete");
  NS_ASSERTION(nsnull != f->OverflowList(), "no overflow list");
  NS_ASSERTION(f->FirstChild()->GetNextInFlow() == f->OverflowList(), "bad next-in-flow");

  
  f->FirstChild()->SetNextInFlow(nsnull);
  f->ClearOverflowList();

  
  
  
  maxSize.width = 100;
  status = f->ResizeReflow(presContext, reflowMetrics, maxSize, nsnull);
  NS_ASSERTION(nsIFrame::frNotComplete == status, "bad status");
  NS_ASSERTION(f->ChildCount() == 1, "bad child count");

  
  if (!f->LastContentIsComplete()) {
    printf("Splittable: last content is NOT complete (#4b)\n");
    return PR_FALSE;
  }

  
  

  
  
  InlineFrame* f1 = new InlineFrame(b, 0, nsnull);

  f1->SetStyleContext(f->GetStyleContext(presContext));
  f->SetNextSibling(f1);
  f->SetNextInFlow(f1);
  f1->SetPrevInFlow(f);

  
  maxSize.width = 150;
  status = f1->ResizeReflow(presContext, reflowMetrics, maxSize, nsnull);
  NS_ASSERTION(nsIFrame::frNotComplete == status, "bad status");
  NS_ASSERTION((f1->GetFirstContentOffset() == 1) && (f1->GetLastContentOffset() == 1), "bad offsets");
  NS_ASSERTION(!f1->LastContentIsComplete(), "should not be complete");

  
  
  
  NS_ASSERTION(nsnull == f1->OverflowList(), "unexpected overflow list");
  InlineFrame* f2 = new InlineFrame(b, 0, nsnull);

  f2->SetStyleContext(f->GetStyleContext(presContext));
  f1->SetNextSibling(f2);
  f1->SetNextInFlow(f2);
  f2->SetPrevInFlow(f1);

  
  maxSize.width = 1000;
  status = f2->ResizeReflow(presContext, reflowMetrics, maxSize, nsnull);
  NS_ASSERTION(nsIFrame::frComplete == status, "bad status");

  
  
  if (f2->FirstChild()->GetPrevInFlow() != f1->FirstChild()) {
    printf("Splittable: child frame bad prev-in-flow (#5)\n");
    return PR_FALSE;
  }

  
  
  if ((f2->FirstChild()->GetIndexInParent() != f1->FirstChild()->GetIndexInParent()) ||
      (f2->GetFirstContentOffset() != f1->GetFirstContentOffset())) {
    printf("Splittable: bad content offset (#5)\n");
    return PR_FALSE;
  }

  
  if ((nsnull != f2->FirstChild()->GetNextInFlow()) || (f2->ChildCount() != 2)) {
    printf("Splittable: bad continuing frame (#5)\n");
    return PR_FALSE;
  }

  
  

  
  
  
  
  
  maxSize.width = 1000;
  status = f->ResizeReflow(presContext, reflowMetrics, maxSize, nsnull);
  NS_ASSERTION(nsIFrame::frComplete == status, "bad status");
  NS_ASSERTION((f->ChildCount() == 3) && (f->GetLastContentOffset() == 2), "bad count");

  f1->SetFirstContentOffset(f->NextChildOffset());  
  f2->SetFirstContentOffset(f->NextChildOffset());  

  
  maxSize.width = 50;
  status = f->ResizeReflow(presContext, reflowMetrics, maxSize, nsnull);
  NS_ASSERTION(nsIFrame::frNotComplete == status, "bad status");

  
  if (f->ChildCount() != 1) {
    printf("Splittable: bad child count (#6): %d\n", f->ChildCount());
    return PR_FALSE;
  }

  
  if ((f->GetLastContentOffset() != 0) || f->LastContentIsComplete()) {
    printf("Splittable: bad content mapping (#6)\n");
    return PR_FALSE;
  }

  
  
  if (f1->ChildCount() != 3) {
    printf("Splittable: continuing frame bad child count (#6): %d\n",
           f1->ChildCount());
    return PR_FALSE;
  }
  if (f1->FirstChild()->GetPrevInFlow() != f->LastChild()) {
    printf("Splittable: continuing frame bad child flow (#6)\n");
    return PR_FALSE;
  }

  
  if ((f1->GetFirstContentOffset() != 0) || (f1->GetLastContentOffset() != 2)) {
    printf("Splittable: continuing frame bad mapping (#6)\n");
    return PR_FALSE;
  }

  
  

  
  
  
  
  maxSize.width = 1000;
  status = f->ResizeReflow(presContext, reflowMetrics, maxSize, nsnull);
  NS_ASSERTION(nsIFrame::frComplete == status, "bad status");
  NS_ASSERTION((f->ChildCount() == 3) && (f->GetLastContentOffset() == 2), "bad count");

  f1->SetFirstContentOffset(f->NextChildOffset());  
  f2->SetFirstContentOffset(f->NextChildOffset());  

  
  maxSize.width = 200;
  status = f->ResizeReflow(presContext, reflowMetrics, maxSize, nsnull);
  NS_ASSERTION(nsIFrame::frNotComplete == status, "bad status");

  
  if (f->ChildCount() != 2) {
    printf("Splittable: bad child count (#7): %d\n", f->ChildCount());
    return PR_FALSE;
  }

  
  if ((f->GetLastContentOffset() != 1) || (f->LastContentIsComplete())) {
    printf("Splittable: bad mapping (#7)\n");
    return PR_FALSE;
  }

  
  if (f1->ChildCount() != 2) {
    printf("Splittable: continuing frame bad child count (#7): %d\n", f1->ChildCount());
    return PR_FALSE;
  }

  
  if ((f1->GetFirstContentOffset() != 1) || (f1->GetLastContentOffset() != 2)) {
    printf("Splittable: continuing frame bad mapping (#7)\n");
    return PR_FALSE;
  }

  
  
  maxSize.width = 200;
  status = f1->ResizeReflow(presContext, reflowMetrics, maxSize, nsnull);
  NS_ASSERTION(nsIFrame::frNotComplete == status, "bad status");

  
  if ((f1->ChildCount() != 1) || (f1->GetLastContentOffset() != 1)) {
    printf("Splittable: continuing frame bad child count or mapping (#7)\n");
    return PR_FALSE;
  }

  
  
  if ((f2->ChildCount() != 1) ||
      (f2->GetFirstContentOffset() != 2) || (f2->GetLastContentOffset() != 2)) {
    printf("Splittable: last continuing frame bad child count or mapping (#7)\n");
    return PR_FALSE;
  }

  
  
  
  
  maxSize.width = 1000;
  status = f->ResizeReflow(presContext, reflowMetrics, maxSize, nsnull);

  
  if ((nsIFrame::frComplete != status) || (f->ChildCount() != 3)) {
    printf("Splittable: first inline frame does not map all the child frames (#7)\n");
    return PR_FALSE;
  }

  NS_RELEASE(b);
  return PR_TRUE;
}








static PRBool
TestMaxElementSize(nsPresContext* presContext)
{
  
  nsIContent* b;
  NS_NewHTMLContainer(&b, NS_NewAtom("span"));

  
  b->AppendChildTo(new FixedSizeContent(100, 100));
  b->AppendChildTo(new FixedSizeContent(300, 300));
  b->AppendChildTo(new FixedSizeContent(200, 200));

  
  
  InlineFrame*      f = new InlineFrame(b, 0, nsnull);
  nsRefPtr<nsStyleContext> styleContext;
  styleContext = presContext->StyleSet()->ResolveStyleFor(b, nsnull);

  f->SetStyleContext(styleContext);

  
  

  
  
  
  
  
  nsSize                  maxElementSize(0, 0);
  nsReflowMetrics         reflowMetrics;
  nsSize                  maxSize(1000, 1000);
  nsIFrame::ReflowStatus  status;

  status = f->ResizeReflow(presContext, reflowMetrics, maxSize, &maxElementSize);

  
  NS_ASSERTION(nsIFrame::frComplete == status, "isn't complete");
  if ((maxElementSize.width != f->MaxChildWidth()) ||
      (maxElementSize.height != f->MaxChildHeight())) {
    printf("MaxElementSize: wrong result in reflow unmapped (#1): (%d, %d)\n",
      maxElementSize.width, maxElementSize.height);
    return PR_FALSE;
  }

  
  

  
  
  maxElementSize.SizeTo(0, 0);
  status = f->ResizeReflow(presContext, reflowMetrics, maxSize, &maxElementSize);
  NS_ASSERTION(nsIFrame::frComplete == status, "isn't complete");

  if ((maxElementSize.width != f->MaxChildWidth()) ||
      (maxElementSize.height != f->MaxChildHeight())) {
    printf("MaxElementSize: wrong result in reflow mapped (#2): (%d, %d)\n",
      maxElementSize.width, maxElementSize.height);
    return PR_FALSE;
  }

  
  

  
  
  nsIFrame* maxChild = f->ChildAt(1);
  nsPoint   origin;

  maxChild->GetOrigin(origin);
  maxSize.width = origin.x + maxChild->GetWidth();
  maxElementSize.SizeTo(0, 0);
  status = f->ResizeReflow(presContext, reflowMetrics, maxSize, &maxElementSize);

  if ((nsIFrame::frNotComplete != status) || (f->ChildCount() != 2)) {
    printf("MaxElementSize: reflow failed (#3)\n");
    return PR_FALSE;
  }

  
  maxSize.width = 1000;
  maxElementSize.SizeTo(0, 0);
  status = f->ResizeReflow(presContext, reflowMetrics, maxSize, &maxElementSize);
  NS_ASSERTION(nsIFrame::frComplete == status, "isn't complete");

  if ((maxElementSize.width != f->MaxChildWidth()) ||
      (maxElementSize.height != f->MaxChildHeight())) {
    printf("MaxElementSize: wrong result in reflow mapped/unmapped (#3): (%d, %d)\n",
           maxElementSize.width, maxElementSize.height);
    return PR_FALSE;
  }

  
  

  
  
  maxChild = f->ChildAt(1);
  maxChild->GetOrigin(origin);
  maxSize.width = origin.x + maxChild->GetWidth();
  maxElementSize.SizeTo(0, 0);
  status = f->ResizeReflow(presContext, reflowMetrics, maxSize, &maxElementSize);
  NS_ASSERTION(f->ChildCount() == 2, "unexpected child count");

  
  InlineFrame* f1 = new InlineFrame(b, 0, nsnull);

  f1->SetStyleContext(f->GetStyleContext(presContext));
  f->SetNextSibling(f1);
  f->SetNextInFlow(f1);
  f1->SetPrevInFlow(f);
  maxSize.width = 1000;
  status = f1->ResizeReflow(presContext, reflowMetrics, maxSize, &maxElementSize);
  NS_ASSERTION((nsIFrame::frComplete == status) && (f1->ChildCount() == 1), "bad continuing frame");

  
  
  maxElementSize.SizeTo(0, 0);
  status = f->ResizeReflow(presContext, reflowMetrics, maxSize, &maxElementSize);
  NS_ASSERTION(nsIFrame::frComplete == status, "isn't complete");
  NS_ASSERTION(f1->ChildCount() == 0, "pull-up failed");

  if ((maxElementSize.width != f->MaxChildWidth()) ||
      (maxElementSize.height != f->MaxChildHeight())) {
    printf("MaxElementSize: wrong result in reflow mapped/pull-up (#4): (%d, %d)\n",
           maxElementSize.width, maxElementSize.height);
    return PR_FALSE;
  }

  
  f1->SetFirstContentOffset(f->NextChildOffset());  
  maxSize.width = f->FirstChild()->GetWidth();
  status = f->ResizeReflow(presContext, reflowMetrics, maxSize, &maxElementSize);
  NS_ASSERTION(f->ChildCount() == 1, "unexpected child count");
  NS_ASSERTION(f1->ChildCount() == 2, "unexpected child count");

  
  
  maxSize.width = 1000;
  maxElementSize.SizeTo(0, 0);
  status = f->ResizeReflow(presContext, reflowMetrics, maxSize, &maxElementSize);
  NS_ASSERTION(nsIFrame::frComplete == status, "isn't complete");
  NS_ASSERTION(f1->ChildCount() == 0, "pull-up failed");

  if ((maxElementSize.width != f->MaxChildWidth()) ||
      (maxElementSize.height != f->MaxChildHeight())) {
    printf("MaxElementSize: wrong result in reflow mapped/pull-up (#4): (%d, %d)\n",
           maxElementSize.width, maxElementSize.height);
    return PR_FALSE;
  }

  NS_RELEASE(b);
  return PR_TRUE;
}
#endif

int main(int argc, char** argv)
{
#if 0
  
  MyDocument *myDoc = new MyDocument();
  nsPresContext* presContext;
  nsIDeviceContext *dx;
  
  static NS_DEFINE_IID(kDeviceContextCID, NS_DEVICE_CONTEXT_CID);

  nsresult rv = CallCreateInstance(kDeviceContextCID, &dx);

  if (NS_OK == rv) {
    dx->Init(nsull);
     dx->SetDevUnitsToAppUnits(dx->DevUnitsToTwips());
     dx->SetAppUnitsToDevUnits(dx->TwipsToDevUnits());
  }

  NS_NewGalleyContext(&presContext);

  presContext->Init(dx, nsnull);

  
  if (!TestReflowUnmapped(presContext)) {
    return -1;
  }

  
  if (!TestChildrenThatDontFit(presContext)) {
    return -1;
  }

  if (!TestOverflow(presContext)) {
    return -1;
  }

  if (!TestPushingPulling(presContext)) {
    return -1;
  }

  if (!TestSplittableChildren(presContext)) {
    return -1;
  }

  if (!TestMaxElementSize(presContext)) {
    return -1;
  }

  





























  presContext->Release();
  myDoc->Release();
#endif
  return 0;
}
