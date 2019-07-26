




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


void
nsFrameList::Init()
{
  NS_PRECONDITION(!sEmptyList, "Shouldn't be allocated");

  sEmptyList = new nsFrameList();
}

void
nsFrameList::Destroy()
{
  NS_PRECONDITION(this != sEmptyList, "Shouldn't Destroy() sEmptyList");

  DestroyFrames();
  delete this;
}

void
nsFrameList::DestroyFrom(nsIFrame* aDestructRoot)
{
  NS_PRECONDITION(this != sEmptyList, "Shouldn't Destroy() sEmptyList");

  DestroyFramesFrom(aDestructRoot);
  delete this;
}

void
nsFrameList::DestroyFrames()
{
  while (nsIFrame* frame = RemoveFirstChild()) {
    frame->Destroy();
  }
  mLastChild = nullptr;
}

void
nsFrameList::DestroyFramesFrom(nsIFrame* aDestructRoot)
{
  NS_PRECONDITION(aDestructRoot, "Missing destruct root");

  while (nsIFrame* frame = RemoveFirstChild()) {
    frame->DestroyFrom(aDestructRoot);
  }
  mLastChild = nullptr;
}

void
nsFrameList::SetFrames(nsIFrame* aFrameList)
{
  NS_PRECONDITION(!mFirstChild, "Losing frames");

  mFirstChild = aFrameList;
  mLastChild = nsLayoutUtils::GetLastSibling(mFirstChild);
}

void
nsFrameList::RemoveFrame(nsIFrame* aFrame)
{
  NS_PRECONDITION(aFrame, "null ptr");
#ifdef DEBUG_FRAME_LIST
  
  NS_PRECONDITION(ContainsFrame(aFrame), "wrong list");
#endif

  nsIFrame* nextFrame = aFrame->GetNextSibling();
  if (aFrame == mFirstChild) {
    mFirstChild = nextFrame;
    aFrame->SetNextSibling(nullptr);
    if (!nextFrame) {
      mLastChild = nullptr;
    }
  }
  else {
    nsIFrame* prevSibling = aFrame->GetPrevSibling();
    NS_ASSERTION(prevSibling && prevSibling->GetNextSibling() == aFrame,
                 "Broken frame linkage");
    prevSibling->SetNextSibling(nextFrame);
    aFrame->SetNextSibling(nullptr);
    if (!nextFrame) {
      mLastChild = prevSibling;
    }
  }
}

bool
nsFrameList::RemoveFrameIfPresent(nsIFrame* aFrame)
{
  NS_PRECONDITION(aFrame, "null ptr");

  for (Enumerator e(*this); !e.AtEnd(); e.Next()) {
    if (e.get() == aFrame) {
      RemoveFrame(aFrame);
      return true;
    }
  }
  return false;
}

nsFrameList
nsFrameList::RemoveFramesAfter(nsIFrame* aAfterFrame)
{
  if (!aAfterFrame) {
    nsFrameList result;
    result.InsertFrames(nullptr, nullptr, *this);
    return result;
  }

  NS_PRECONDITION(NotEmpty(), "illegal operation on empty list");
#ifdef DEBUG_FRAME_LIST
  NS_PRECONDITION(ContainsFrame(aAfterFrame), "wrong list");
#endif

  nsIFrame* tail = aAfterFrame->GetNextSibling();
  
  nsIFrame* oldLastChild = mLastChild;
  mLastChild = aAfterFrame;
  aAfterFrame->SetNextSibling(nullptr);
  return nsFrameList(tail, tail ? oldLastChild : nullptr);
}

nsIFrame*
nsFrameList::RemoveFirstChild()
{
  if (mFirstChild) {
    nsIFrame* firstChild = mFirstChild;
    RemoveFrame(firstChild);
    return firstChild;
  }
  return nullptr;
}

void
nsFrameList::DestroyFrame(nsIFrame* aFrame)
{
  NS_PRECONDITION(aFrame, "null ptr");
  RemoveFrame(aFrame);
  aFrame->Destroy();
}

bool
nsFrameList::DestroyFrameIfPresent(nsIFrame* aFrame)
{
  NS_PRECONDITION(aFrame, "null ptr");

  if (RemoveFrameIfPresent(aFrame)) {
    aFrame->Destroy();
    return true;
  }
  return false;
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
#ifdef DEBUG_FRAME_LIST
  
  NS_ASSERTION(!aPrevSibling || ContainsFrame(aPrevSibling),
               "prev sibling is not on this list");
#endif

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

  VerifyList();

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
  NS_PRECONDITION(aLink.mEnd == nullptr,
                  "Unexpected mEnd for frame link enumerator");

  nsIFrame* prev = aLink.PrevFrame();
  nsIFrame* newFirstFrame = nullptr;
  if (prev) {
    
    prev->SetNextSibling(nullptr);
    newFirstFrame = mFirstChild;
    mFirstChild = aLink.NextFrame();
    if (!mFirstChild) { 
      mLastChild = nullptr;
    }

    
    aLink.mPrev = nullptr;
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
  NS_PRECONDITION(aLink.mEnd == nullptr,
                  "Unexpected mEnd for frame link enumerator");

  nsIFrame* prev = aLink.PrevFrame();
  nsIFrame* newFirstFrame;
  nsIFrame* newLastFrame;
  if (prev) {
    
    prev->SetNextSibling(nullptr);
    newFirstFrame = aLink.NextFrame();
    newLastFrame = newFirstFrame ? mLastChild : nullptr;
    mLastChild = prev;
  } else {
    
    newFirstFrame = mFirstChild;
    newLastFrame = mLastChild;
    Clear();
  }

  
  aLink.mFrame = nullptr;

  NS_POSTCONDITION(aLink.AtEnd(), "What's going on here?");

  return nsFrameList(newFirstFrame, newLastFrame);
}

nsIFrame*
nsFrameList::FrameAt(int32_t aIndex) const
{
  NS_PRECONDITION(aIndex >= 0, "invalid arg");
  if (aIndex < 0) return nullptr;
  nsIFrame* frame = mFirstChild;
  while ((aIndex-- > 0) && frame) {
    frame = frame->GetNextSibling();
  }
  return frame;
}

int32_t
nsFrameList::IndexOf(nsIFrame* aFrame) const
{
  int32_t count = 0;
  for (nsIFrame* f = mFirstChild; f; f = f->GetNextSibling()) {
    if (f == aFrame)
      return count;
    ++count;
  }
  return -1;
}

bool
nsFrameList::ContainsFrame(const nsIFrame* aFrame) const
{
  NS_PRECONDITION(aFrame, "null ptr");

  nsIFrame* frame = mFirstChild;
  while (frame) {
    if (frame == aFrame) {
      return true;
    }
    frame = frame->GetNextSibling();
  }
  return false;
}

int32_t
nsFrameList::GetLength() const
{
  int32_t count = 0;
  nsIFrame* frame = mFirstChild;
  while (frame) {
    count++;
    frame = frame->GetNextSibling();
  }
  return count;
}

void
nsFrameList::ApplySetParent(nsIFrame* aParent) const
{
  NS_ASSERTION(aParent, "null ptr");

  for (nsIFrame* f = FirstChild(); f; f = f->GetNextSibling()) {
    f->SetParent(aParent);
  }
}

 void
nsFrameList::UnhookFrameFromSiblings(nsIFrame* aFrame)
{
  MOZ_ASSERT(aFrame->GetPrevSibling() && aFrame->GetNextSibling());
  nsIFrame* const nextSibling = aFrame->GetNextSibling();
  nsIFrame* const prevSibling = aFrame->GetPrevSibling();
  aFrame->SetNextSibling(nullptr);
  prevSibling->SetNextSibling(nextSibling);
  MOZ_ASSERT(!aFrame->GetPrevSibling() && !aFrame->GetNextSibling());
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
    return nullptr;
  
  nsIFrame* parent = mFirstChild->GetParent();
  if (!parent)
    return aFrame ? aFrame->GetPrevSibling() : LastChild();

  nsBidiLevel baseLevel = nsBidiPresUtils::GetFrameBaseLevel(mFirstChild);  

  nsAutoLineIterator iter = parent->GetLineIterator();
  if (!iter) { 
    
    if (parent->GetType() == nsGkAtoms::lineFrame) {
      
      if (baseLevel == NSBIDI_LTR) {
        return nsBidiPresUtils::GetFrameToLeftOf(aFrame, mFirstChild, -1);
      } else { 
        return nsBidiPresUtils::GetFrameToRightOf(aFrame, mFirstChild, -1);
      }
    } else {
      
      nsBidiLevel frameEmbeddingLevel = nsBidiPresUtils::GetFrameEmbeddingLevel(mFirstChild);
      if ((frameEmbeddingLevel & 1) == (baseLevel & 1)) {
        return aFrame ? aFrame->GetPrevSibling() : LastChild();
      } else {
        return aFrame ? aFrame->GetNextSibling() : mFirstChild;
      }    
    }
  }

  
  

  int32_t thisLine;
  if (aFrame) {
    thisLine = iter->FindLineContaining(aFrame);
    if (thisLine < 0)
      return nullptr;
  } else {
    thisLine = iter->GetNumLines();
  }

  nsIFrame* frame = nullptr;
  nsIFrame* firstFrameOnLine;
  int32_t numFramesOnLine;
  nsRect lineBounds;
  uint32_t lineFlags;

  if (aFrame) {
    iter->GetLine(thisLine, &firstFrameOnLine, &numFramesOnLine, lineBounds, &lineFlags);

    if (baseLevel == NSBIDI_LTR) {
      frame = nsBidiPresUtils::GetFrameToLeftOf(aFrame, firstFrameOnLine, numFramesOnLine);
    } else { 
      frame = nsBidiPresUtils::GetFrameToRightOf(aFrame, firstFrameOnLine, numFramesOnLine);
    }
  }

  if (!frame && thisLine > 0) {
    
    iter->GetLine(thisLine - 1, &firstFrameOnLine, &numFramesOnLine, lineBounds, &lineFlags);

    if (baseLevel == NSBIDI_LTR) {
      frame = nsBidiPresUtils::GetFrameToLeftOf(nullptr, firstFrameOnLine, numFramesOnLine);
    } else { 
      frame = nsBidiPresUtils::GetFrameToRightOf(nullptr, firstFrameOnLine, numFramesOnLine);
    }
  }
  return frame;
}

nsIFrame*
nsFrameList::GetNextVisualFor(nsIFrame* aFrame) const
{
  if (!mFirstChild)
    return nullptr;
  
  nsIFrame* parent = mFirstChild->GetParent();
  if (!parent)
    return aFrame ? aFrame->GetPrevSibling() : mFirstChild;

  nsBidiLevel baseLevel = nsBidiPresUtils::GetFrameBaseLevel(mFirstChild);
  
  nsAutoLineIterator iter = parent->GetLineIterator();
  if (!iter) { 
    
    if (parent->GetType() == nsGkAtoms::lineFrame) {
      
      if (baseLevel == NSBIDI_LTR) {
        return nsBidiPresUtils::GetFrameToRightOf(aFrame, mFirstChild, -1);
      } else { 
        return nsBidiPresUtils::GetFrameToLeftOf(aFrame, mFirstChild, -1);
      }
    } else {
      
      nsBidiLevel frameEmbeddingLevel = nsBidiPresUtils::GetFrameEmbeddingLevel(mFirstChild);
      if ((frameEmbeddingLevel & 1) == (baseLevel & 1)) {
        return aFrame ? aFrame->GetNextSibling() : mFirstChild;
      } else {
        return aFrame ? aFrame->GetPrevSibling() : LastChild();
      }
    }
  }

  
  
  
  int32_t thisLine;
  if (aFrame) {
    thisLine = iter->FindLineContaining(aFrame);
    if (thisLine < 0)
      return nullptr;
  } else {
    thisLine = -1;
  }

  nsIFrame* frame = nullptr;
  nsIFrame* firstFrameOnLine;
  int32_t numFramesOnLine;
  nsRect lineBounds;
  uint32_t lineFlags;

  if (aFrame) {
    iter->GetLine(thisLine, &firstFrameOnLine, &numFramesOnLine, lineBounds, &lineFlags);
    
    if (baseLevel == NSBIDI_LTR) {
      frame = nsBidiPresUtils::GetFrameToRightOf(aFrame, firstFrameOnLine, numFramesOnLine);
    } else { 
      frame = nsBidiPresUtils::GetFrameToLeftOf(aFrame, firstFrameOnLine, numFramesOnLine);
    }
  }
  
  int32_t numLines = iter->GetNumLines();
  if (!frame && thisLine < numLines - 1) {
    
    iter->GetLine(thisLine + 1, &firstFrameOnLine, &numFramesOnLine, lineBounds, &lineFlags);
    
    if (baseLevel == NSBIDI_LTR) {
      frame = nsBidiPresUtils::GetFrameToRightOf(nullptr, firstFrameOnLine, numFramesOnLine);
    } else { 
      frame = nsBidiPresUtils::GetFrameToLeftOf(nullptr, firstFrameOnLine, numFramesOnLine);
    }
  }
  return frame;
}
#endif

#ifdef DEBUG_FRAME_LIST
void
nsFrameList::VerifyList() const
{
  NS_ASSERTION((mFirstChild == nullptr) == (mLastChild == nullptr),
               "bad list state");

  if (IsEmpty()) {
    return;
  }

  
  
  NS_ASSERTION(!mFirstChild->GetPrevSibling(), "bad prev sibling pointer");
  nsIFrame *first = mFirstChild, *second = mFirstChild;
  for (;;) {
    first = first->GetNextSibling();
    second = second->GetNextSibling();
    if (!second) {
      break;
    }
    NS_ASSERTION(second->GetPrevSibling()->GetNextSibling() == second,
                 "bad prev sibling pointer");
    second = second->GetNextSibling();
    if (first == second) {
      
      
      NS_ERROR("loop in frame list.  This will probably hang soon.");
      return;
    }                           
    if (!second) {
      break;
    }
    NS_ASSERTION(second->GetPrevSibling()->GetNextSibling() == second,
                 "bad prev sibling pointer");
  }

  NS_ASSERTION(mLastChild == nsLayoutUtils::GetLastSibling(mFirstChild),
               "bogus mLastChild");
  
  
  
}
#endif
