





#include "gfxUserFontSet.h"
#include "nsFontFaceUtils.h"
#include "nsFontMetrics.h"
#include "nsIFrame.h"
#include "nsLayoutUtils.h"
#include "nsPlaceholderFrame.h"
#include "nsTArray.h"
#include "SVGTextFrame.h"

static bool
StyleContextContainsFont(nsStyleContext* aStyleContext,
                         const gfxUserFontSet* aUserFontSet,
                         const gfxUserFontEntry* aFont)
{
  
  
  if (!aFont) {
    const FontFamilyList& fontlist =
      aStyleContext->StyleFont()->mFont.fontlist;
    return aUserFontSet->ContainsUserFontSetFonts(fontlist);
  }

  
  const nsString& familyName = aFont->FamilyName();
  if (!aStyleContext->StyleFont()->mFont.fontlist.Contains(familyName)) {
    return false;
  }

  
  
  nsRefPtr<nsFontMetrics> fm;
  nsLayoutUtils::GetFontMetricsForStyleContext(aStyleContext,
                                               getter_AddRefs(fm),
                                               1.0f);

  if (fm->GetThebesFontGroup()->ContainsUserFont(aFont)) {
    return true;
  }

  return false;
}

static bool
FrameUsesFont(nsIFrame* aFrame, const gfxUserFontEntry* aFont)
{
  
  gfxUserFontSet* ufs = aFrame->PresContext()->GetUserFontSet();
  if (StyleContextContainsFont(aFrame->StyleContext(), ufs, aFont)) {
    return true;
  }

  
  int32_t contextIndex = 0;
  for (nsStyleContext* extraContext;
       (extraContext = aFrame->GetAdditionalStyleContext(contextIndex));
       ++contextIndex) {
    if (StyleContextContainsFont(extraContext, ufs, aFont)) {
      return true;
    }
  }

  return false;
}

static void
ScheduleReflow(nsIPresShell* aShell, nsIFrame* aFrame)
{
  nsIFrame* f = aFrame;
  if (f->IsFrameOfType(nsIFrame::eSVG) || f->IsSVGText()) {
    
    
    
    
    
    
    if (f->GetStateBits() & NS_FRAME_IS_NONDISPLAY) {
      while (f) {
        if (!(f->GetStateBits() & NS_FRAME_IS_NONDISPLAY)) {
          if (NS_SUBTREE_DIRTY(f)) {
            
            
            
            return;
          }
          if (f->GetStateBits() & NS_STATE_IS_OUTER_SVG ||
              !(f->IsFrameOfType(nsIFrame::eSVG) || f->IsSVGText())) {
            break;
          }
          f->AddStateBits(NS_FRAME_HAS_DIRTY_CHILDREN);
        }
        f = f->GetParent();
      }
      MOZ_ASSERT(f, "should have found an ancestor frame to reflow");
    }
  }

  aShell->FrameNeedsReflow(f, nsIPresShell::eStyleChange, NS_FRAME_IS_DIRTY);
}

 void
nsFontFaceUtils::MarkDirtyForFontChange(nsIFrame* aSubtreeRoot,
                                        const gfxUserFontEntry* aFont)
{
  nsAutoTArray<nsIFrame*, 4> subtrees;
  subtrees.AppendElement(aSubtreeRoot);

  nsIPresShell* ps = aSubtreeRoot->PresContext()->PresShell();

  
  
  do {
    nsIFrame* subtreeRoot = subtrees.ElementAt(subtrees.Length() - 1);
    subtrees.RemoveElementAt(subtrees.Length() - 1);

    
    nsAutoTArray<nsIFrame*, 32> stack;
    stack.AppendElement(subtreeRoot);

    do {
      nsIFrame* f = stack.ElementAt(stack.Length() - 1);
      stack.RemoveElementAt(stack.Length() - 1);

      
      
      if (FrameUsesFont(f, aFont)) {
        ScheduleReflow(ps, f);
      } else {
        if (f->GetType() == nsGkAtoms::placeholderFrame) {
          nsIFrame* oof = nsPlaceholderFrame::GetRealFrameForPlaceholder(f);
          if (!nsLayoutUtils::IsProperAncestorFrame(subtreeRoot, oof)) {
            
            subtrees.AppendElement(oof);
          }
        }

        nsIFrame::ChildListIterator lists(f);
        for (; !lists.IsDone(); lists.Next()) {
          nsFrameList::Enumerator childFrames(lists.CurrentList());
          for (; !childFrames.AtEnd(); childFrames.Next()) {
            nsIFrame* kid = childFrames.get();
            stack.AppendElement(kid);
          }
        }
      }
    } while (!stack.IsEmpty());
  } while (!subtrees.IsEmpty());
}
