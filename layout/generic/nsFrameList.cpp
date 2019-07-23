





































#include "nsFrameList.h"
#include "nsIFrame.h"
#include "nsLayoutUtils.h"

#ifdef IBMBIDI
#include "nsCOMPtr.h"
#include "nsGkAtoms.h"
#include "nsILineIterator.h"
#include "nsBidiPresUtils.h"
#endif 

const nsFrameList* nsFrameList::sEmptyList;

nsFrameList::nsFrameList(nsIFrame* aFirstFrame)
  : mFirstChild(aFirstFrame)
  , mLastChild(nsLayoutUtils::GetLastSibling(aFirstFrame))
{
  MOZ_COUNT_CTOR(nsFrameList);
}


nsresult
nsFrameList::Init()
{
  NS_PRECONDITION(!sEmptyList, "Shouldn't be allocated");

  sEmptyList = new nsFrameList();
  if (!sEmptyList)
    return NS_ERROR_OUT_OF_MEMORY;

  return NS_OK;
}

void
nsFrameList::Destroy()
{
  NS_PRECONDITION(this != sEmptyList, "Shouldn't Destroy() sEmptyList");

  DestroyFrames();
  delete this;
}

void
nsFrameList::DestroyFrames()
{
  nsIFrame* next;
  for (nsIFrame* frame = mFirstChild; frame; frame = next) {
    next = frame->GetNextSibling();
    frame->Destroy();
    mFirstChild = next;
  }

  mLastChild = nsnull;
}

void
nsFrameList::SetFrames(nsIFrame* aFrameList)
{
  NS_PRECONDITION(!mFirstChild, "Losing frames");

  mFirstChild = aFrameList;
  mLastChild = nsLayoutUtils::GetLastSibling(mFirstChild);
}

PRBool
nsFrameList::RemoveFrame(nsIFrame* aFrame, nsIFrame* aPrevSiblingHint)
{
  NS_PRECONDITION(aFrame, "null ptr");

  nsIFrame* nextFrame = aFrame->GetNextSibling();
  if (aFrame == mFirstChild) {
    mFirstChild = nextFrame;
    aFrame->SetNextSibling(nsnull);
    if (!nextFrame) {
      mLastChild = nsnull;
    }
    return PR_TRUE;
  }
  else {
    nsIFrame* prevSibling = aPrevSiblingHint;
    if (!prevSibling || prevSibling->GetNextSibling() != aFrame) {
      prevSibling = GetPrevSiblingFor(aFrame);
    }
    if (prevSibling) {
      prevSibling->SetNextSibling(nextFrame);
      aFrame->SetNextSibling(nsnull);
      if (!nextFrame) {
        mLastChild = prevSibling;
      }
      return PR_TRUE;
    }
  }

  return PR_FALSE;
}

nsFrameList
nsFrameList::RemoveFramesAfter(nsIFrame* aAfterFrame)
{
  NS_PRECONDITION(NotEmpty(), "illegal operation on empty list");
  NS_PRECONDITION(ContainsFrame(aAfterFrame), "wrong list");

  nsIFrame* tail = aAfterFrame->GetNextSibling();
  
  nsIFrame* oldLastChild = mLastChild;
  mLastChild = aAfterFrame;
  aAfterFrame->SetNextSibling(nsnull);
  return nsFrameList(tail, tail ? oldLastChild : nsnull);
}

PRBool
nsFrameList::RemoveFirstChild()
{
  if (mFirstChild) {
    RemoveFrame(mFirstChild);
    return PR_TRUE;
  }
  return PR_FALSE;
}

PRBool
nsFrameList::DestroyFrame(nsIFrame* aFrame, nsIFrame* aPrevSiblingHint)
{
  NS_PRECONDITION(nsnull != aFrame, "null ptr");
  if (RemoveFrame(aFrame, aPrevSiblingHint)) {
    aFrame->Destroy();
    return PR_TRUE;
  }
  return PR_FALSE;
}

nsFrameList::Slice
nsFrameList::InsertFrames(nsIFrame* aParent, nsIFrame* aPrevSibling,
                          nsFrameList& aFrameList)
{
  NS_PRECONDITION(aFrameList.NotEmpty(), "Unexpected empty list");

  if (aParent) {
    aFrameList.ApplySetParent(aParent);
  }

  NS_ASSERTION(IsEmpty() ||
               FirstChild()->GetParent() == aFrameList.FirstChild()->GetParent(),
               "frame to add has different parent");
  NS_ASSERTION(!aPrevSibling ||
               aPrevSibling->GetParent() == aFrameList.FirstChild()->GetParent(),
               "prev sibling has different parent");
  NS_ASSERTION(!aPrevSibling || ContainsFrame(aPrevSibling),
               "prev sibling is not on this list");

  nsIFrame* firstNewFrame = aFrameList.FirstChild();
  nsIFrame* nextSibling;
  if (aPrevSibling) {
    nextSibling = aPrevSibling->GetNextSibling();
    aPrevSibling->SetNextSibling(firstNewFrame);
  }
  else {
    nextSibling = mFirstChild;
    mFirstChild = firstNewFrame;
  }

  nsIFrame* lastNewFrame = aFrameList.LastChild();
  lastNewFrame->SetNextSibling(nextSibling);
  if (!nextSibling) {
    mLastChild = lastNewFrame;
  }

#ifdef DEBUG
  VerifyList();
#endif

  aFrameList.Clear();
  return Slice(*this, firstNewFrame, nextSibling);
}

nsFrameList
nsFrameList::ExtractHead(FrameLinkEnumerator& aLink)
{
  NS_PRECONDITION(&aLink.List() == this, "Unexpected list");
  NS_PRECONDITION(!aLink.PrevFrame() ||
                  aLink.PrevFrame()->GetNextSibling() ==
                    aLink.NextFrame(),
                  "Unexpected PrevFrame()");
  NS_PRECONDITION(aLink.PrevFrame() ||
                  aLink.NextFrame() == FirstChild(),
                  "Unexpected NextFrame()");
  NS_PRECONDITION(!aLink.PrevFrame() ||
                  aLink.NextFrame() != FirstChild(),
                  "Unexpected NextFrame()");
  NS_PRECONDITION(aLink.mEnd == nsnull,
                  "Unexpected mEnd for frame link enumerator");

  nsIFrame* prev = aLink.PrevFrame();
  nsIFrame* newFirstFrame = nsnull;
  if (prev) {
    
    prev->SetNextSibling(nsnull);
    newFirstFrame = mFirstChild;
    mFirstChild = aLink.NextFrame();
    if (!mFirstChild) { 
      mLastChild = nsnull;
    }

    
    aLink.mPrev = nsnull;
  }
  

  return nsFrameList(newFirstFrame, prev);
}

nsFrameList
nsFrameList::ExtractTail(FrameLinkEnumerator& aLink)
{
  NS_PRECONDITION(&aLink.List() == this, "Unexpected list");
  NS_PRECONDITION(!aLink.PrevFrame() ||
                  aLink.PrevFrame()->GetNextSibling() ==
                    aLink.NextFrame(),
                  "Unexpected PrevFrame()");
  NS_PRECONDITION(aLink.PrevFrame() ||
                  aLink.NextFrame() == FirstChild(),
                  "Unexpected NextFrame()");
  NS_PRECONDITION(!aLink.PrevFrame() ||
                  aLink.NextFrame() != FirstChild(),
                  "Unexpected NextFrame()");
  NS_PRECONDITION(aLink.mEnd == nsnull,
                  "Unexpected mEnd for frame link enumerator");

  nsIFrame* prev = aLink.PrevFrame();
  nsIFrame* newFirstFrame;
  nsIFrame* newLastFrame;
  if (prev) {
    
    prev->SetNextSibling(nsnull);
    newFirstFrame = aLink.NextFrame();
    newLastFrame = newFirstFrame ? mLastChild : nsnull;
    mLastChild = prev;
  } else {
    
    newFirstFrame = mFirstChild;
    newLastFrame = mLastChild;
    Clear();
  }

  
  aLink.mFrame = nsnull;

  NS_POSTCONDITION(aLink.AtEnd(), "What's going on here?");

  return nsFrameList(newFirstFrame, newLastFrame);
}

nsIFrame*
nsFrameList::FrameAt(PRInt32 aIndex) const
{
  NS_PRECONDITION(aIndex >= 0, "invalid arg");
  if (aIndex < 0) return nsnull;
  nsIFrame* frame = mFirstChild;
  while ((aIndex-- > 0) && frame) {
    frame = frame->GetNextSibling();
  }
  return frame;
}

PRInt32
nsFrameList::IndexOf(nsIFrame* aFrame) const
{
  PRInt32 count = 0;
  for (nsIFrame* f = mFirstChild; f; f = f->GetNextSibling()) {
    if (f == aFrame)
      return count;
    ++count;
  }
  return -1;
}

PRBool
nsFrameList::ContainsFrame(const nsIFrame* aFrame) const
{
  NS_PRECONDITION(aFrame, "null ptr");

  nsIFrame* frame = mFirstChild;
  while (frame) {
    if (frame == aFrame) {
      return PR_TRUE;
    }
    frame = frame->GetNextSibling();
  }
  return PR_FALSE;
}

PRBool
nsFrameList::ContainsFrameBefore(const nsIFrame* aFrame, const nsIFrame* aEnd) const
{
  NS_PRECONDITION(aFrame, "null ptr");

  nsIFrame* frame = mFirstChild;
  while (frame) {
    if (frame == aEnd) {
      return PR_FALSE;
    }
    if (frame == aFrame) {
      return PR_TRUE;
    }
    frame = frame->GetNextSibling();
  }
  return PR_FALSE;
}

PRInt32
nsFrameList::GetLength() const
{
  PRInt32 count = 0;
  nsIFrame* frame = mFirstChild;
  while (frame) {
    count++;
    frame = frame->GetNextSibling();
  }
  return count;
}

static int CompareByContentOrder(const nsIFrame* aF1, const nsIFrame* aF2)
{
  if (aF1->GetContent() != aF2->GetContent()) {
    return nsLayoutUtils::CompareTreePosition(aF1->GetContent(), aF2->GetContent());
  }

  if (aF1 == aF2) {
    return 0;
  }

  const nsIFrame* f;
  for (f = aF2; f; f = f->GetPrevInFlow()) {
    if (f == aF1) {
      
      return -1;
    }
  }
  for (f = aF1; f; f = f->GetPrevInFlow()) {
    if (f == aF2) {
      
      return 1;
    }
  }

  NS_ASSERTION(PR_FALSE, "Frames for same content but not in relative flow order");
  return 0;
}

class CompareByContentOrderComparator
{
  public:
  PRBool Equals(const nsIFrame* aA, const nsIFrame* aB) const {
    return aA == aB;
  }
  PRBool LessThan(const nsIFrame* aA, const nsIFrame* aB) const {
    return CompareByContentOrder(aA, aB) < 0;
  }
};

void
nsFrameList::SortByContentOrder()
{
  if (IsEmpty())
    return;

  nsAutoTArray<nsIFrame*, 8> array;
  nsIFrame* f;
  for (f = mFirstChild; f; f = f->GetNextSibling()) {
    array.AppendElement(f);
  }
  array.Sort(CompareByContentOrderComparator());
  f = mFirstChild = array.ElementAt(0);
  for (PRUint32 i = 1; i < array.Length(); ++i) {
    nsIFrame* ff = array.ElementAt(i);
    f->SetNextSibling(ff);
    f = ff;
  }
  f->SetNextSibling(nsnull);
  mLastChild = f;
}

nsIFrame*
nsFrameList::GetPrevSiblingFor(nsIFrame* aFrame) const
{
  NS_PRECONDITION(aFrame, "null ptr");
  NS_PRECONDITION(mFirstChild, "GetPrevSiblingFor() on empty frame list");

  if (aFrame == mFirstChild) {
    return nsnull;
  }
  nsIFrame* frame = mFirstChild;
  while (frame) {
    nsIFrame* next = frame->GetNextSibling();
    if (next == aFrame) {
      break;
    }
    frame = next;
  }

  NS_POSTCONDITION(frame, "GetPrevSiblingFor() on wrong frame list");
  return frame;
}

void
nsFrameList::ApplySetParent(nsIFrame* aParent) const
{
  NS_ASSERTION(aParent, "null ptr");

  for (nsIFrame* f = FirstChild(); f; f = f->GetNextSibling()) {
    f->SetParent(aParent);
  }
}

#ifdef DEBUG
void
nsFrameList::List(FILE* out) const
{
  fputs("<\n", out);
  for (nsIFrame* frame = mFirstChild; frame;
       frame = frame->GetNextSibling()) {
    frame->List(out, 1);
  }
  fputs(">\n", out);
}
#endif

#ifdef IBMBIDI
nsIFrame*
nsFrameList::GetPrevVisualFor(nsIFrame* aFrame) const
{
  if (!mFirstChild)
    return nsnull;
  
  nsIFrame* parent = mFirstChild->GetParent();
  if (!parent)
    return aFrame ? GetPrevSiblingFor(aFrame) : LastChild();

  nsBidiLevel baseLevel = nsBidiPresUtils::GetFrameBaseLevel(mFirstChild);  
  nsBidiPresUtils* bidiUtils = mFirstChild->PresContext()->GetBidiUtils();

  nsAutoLineIterator iter = parent->GetLineIterator();
  if (!iter) { 
    
    if (parent->GetType() == nsGkAtoms::lineFrame) {
      
      if (baseLevel == NSBIDI_LTR) {
        return bidiUtils->GetFrameToLeftOf(aFrame, mFirstChild, -1);
      } else { 
        return bidiUtils->GetFrameToRightOf(aFrame, mFirstChild, -1);
      }
    } else {
      
      nsBidiLevel frameEmbeddingLevel = nsBidiPresUtils::GetFrameEmbeddingLevel(mFirstChild);
      if ((frameEmbeddingLevel & 1) == (baseLevel & 1)) {
        return aFrame ? GetPrevSiblingFor(aFrame) : LastChild();
      } else {
        return aFrame ? aFrame->GetNextSibling() : mFirstChild;
      }    
    }
  }

  
  

  PRInt32 thisLine;
  if (aFrame) {
    thisLine = iter->FindLineContaining(aFrame);
    if (thisLine < 0)
      return nsnull;
  } else {
    thisLine = iter->GetNumLines();
  }

  nsIFrame* frame = nsnull;
  nsIFrame* firstFrameOnLine;
  PRInt32 numFramesOnLine;
  nsRect lineBounds;
  PRUint32 lineFlags;

  if (aFrame) {
    iter->GetLine(thisLine, &firstFrameOnLine, &numFramesOnLine, lineBounds, &lineFlags);

    if (baseLevel == NSBIDI_LTR) {
      frame = bidiUtils->GetFrameToLeftOf(aFrame, firstFrameOnLine, numFramesOnLine);
    } else { 
      frame = bidiUtils->GetFrameToRightOf(aFrame, firstFrameOnLine, numFramesOnLine);
    }
  }

  if (!frame && thisLine > 0) {
    
    iter->GetLine(thisLine - 1, &firstFrameOnLine, &numFramesOnLine, lineBounds, &lineFlags);

    if (baseLevel == NSBIDI_LTR) {
      frame = bidiUtils->GetFrameToLeftOf(nsnull, firstFrameOnLine, numFramesOnLine);
    } else { 
      frame = bidiUtils->GetFrameToRightOf(nsnull, firstFrameOnLine, numFramesOnLine);
    }
  }
  return frame;
}

nsIFrame*
nsFrameList::GetNextVisualFor(nsIFrame* aFrame) const
{
  if (!mFirstChild)
    return nsnull;
  
  nsIFrame* parent = mFirstChild->GetParent();
  if (!parent)
    return aFrame ? GetPrevSiblingFor(aFrame) : mFirstChild;

  nsBidiLevel baseLevel = nsBidiPresUtils::GetFrameBaseLevel(mFirstChild);
  nsBidiPresUtils* bidiUtils = mFirstChild->PresContext()->GetBidiUtils();
  
  nsAutoLineIterator iter = parent->GetLineIterator();
  if (!iter) { 
    
    if (parent->GetType() == nsGkAtoms::lineFrame) {
      
      if (baseLevel == NSBIDI_LTR) {
        return bidiUtils->GetFrameToRightOf(aFrame, mFirstChild, -1);
      } else { 
        return bidiUtils->GetFrameToLeftOf(aFrame, mFirstChild, -1);
      }
    } else {
      
      nsBidiLevel frameEmbeddingLevel = nsBidiPresUtils::GetFrameEmbeddingLevel(mFirstChild);
      if ((frameEmbeddingLevel & 1) == (baseLevel & 1)) {
        return aFrame ? aFrame->GetNextSibling() : mFirstChild;
      } else {
        return aFrame ? GetPrevSiblingFor(aFrame) : LastChild();
      }
    }
  }

  
  
  
  PRInt32 thisLine;
  if (aFrame) {
    thisLine = iter->FindLineContaining(aFrame);
    if (thisLine < 0)
      return nsnull;
  } else {
    thisLine = -1;
  }

  nsIFrame* frame = nsnull;
  nsIFrame* firstFrameOnLine;
  PRInt32 numFramesOnLine;
  nsRect lineBounds;
  PRUint32 lineFlags;

  if (aFrame) {
    iter->GetLine(thisLine, &firstFrameOnLine, &numFramesOnLine, lineBounds, &lineFlags);
    
    if (baseLevel == NSBIDI_LTR) {
      frame = bidiUtils->GetFrameToRightOf(aFrame, firstFrameOnLine, numFramesOnLine);
    } else { 
      frame = bidiUtils->GetFrameToLeftOf(aFrame, firstFrameOnLine, numFramesOnLine);
    }
  }
  
  PRInt32 numLines = iter->GetNumLines();
  if (!frame && thisLine < numLines - 1) {
    
    iter->GetLine(thisLine + 1, &firstFrameOnLine, &numFramesOnLine, lineBounds, &lineFlags);
    
    if (baseLevel == NSBIDI_LTR) {
      frame = bidiUtils->GetFrameToRightOf(nsnull, firstFrameOnLine, numFramesOnLine);
    } else { 
      frame = bidiUtils->GetFrameToLeftOf(nsnull, firstFrameOnLine, numFramesOnLine);
    }
  }
  return frame;
}
#endif

#ifdef DEBUG
void
nsFrameList::VerifyList() const
{
  NS_ASSERTION((mFirstChild == nsnull) == (mLastChild == nsnull),
               "bad list state");

  if (IsEmpty()) {
    return;
  }

  
  
  nsIFrame *first = mFirstChild, *second = mFirstChild;
  do {
    first = first->GetNextSibling();
    second = second->GetNextSibling();
    if (!second) {
      break;
    }
    second = second->GetNextSibling();
    if (first == second) {
      
      
      NS_ERROR("loop in frame list.  This will probably hang soon.");
      return;
    }                           
  } while (first && second);

  NS_ASSERTION(mLastChild == nsLayoutUtils::GetLastSibling(mFirstChild),
               "bogus mLastChild");
  
  
  
}
#endif
