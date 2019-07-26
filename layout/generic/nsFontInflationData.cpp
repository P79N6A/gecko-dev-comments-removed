






#include "nsFontInflationData.h"
#include "FramePropertyTable.h"
#include "nsTextFragment.h"
#include "nsIFormControlFrame.h"
#include "nsTextControlFrame.h"
#include "nsListControlFrame.h"
#include "nsComboboxControlFrame.h"
#include "nsHTMLReflowState.h"
#include "nsTextFrameUtils.h"

using namespace mozilla;
using namespace mozilla::layout;

static void
DestroyFontInflationData(void *aPropertyValue)
{
  delete static_cast<nsFontInflationData*>(aPropertyValue);
}

NS_DECLARE_FRAME_PROPERTY(FontInflationDataProperty, DestroyFontInflationData)

 nsFontInflationData*
nsFontInflationData::FindFontInflationDataFor(const nsIFrame *aFrame)
{
  
  const nsIFrame *bfc = FlowRootFor(aFrame);
  NS_ASSERTION(bfc->GetStateBits() & NS_FRAME_FONT_INFLATION_FLOW_ROOT,
               "should have found a flow root");

  return static_cast<nsFontInflationData*>(
             bfc->Properties().Get(FontInflationDataProperty()));
}

 bool
nsFontInflationData::UpdateFontInflationDataWidthFor(const nsHTMLReflowState& aReflowState)
{
  nsIFrame *bfc = aReflowState.frame;
  NS_ASSERTION(bfc->GetStateBits() & NS_FRAME_FONT_INFLATION_FLOW_ROOT,
               "should have been given a flow root");
  FrameProperties bfcProps(bfc->Properties());
  nsFontInflationData *data = static_cast<nsFontInflationData*>(
                                bfcProps.Get(FontInflationDataProperty()));
  bool oldInflationEnabled;
  nscoord oldNCAWidth;
  if (data) {
    oldNCAWidth = data->mNCAWidth;
    oldInflationEnabled = data->mInflationEnabled;
  } else {
    data = new nsFontInflationData(bfc);
    bfcProps.Set(FontInflationDataProperty(), data);
    oldNCAWidth = -1;
    oldInflationEnabled = true; 
  }

  data->UpdateWidth(aReflowState);

  if (oldInflationEnabled != data->mInflationEnabled)
    return true;

  return oldInflationEnabled &&
         oldNCAWidth != data->mNCAWidth;
}

 void
nsFontInflationData::MarkFontInflationDataTextDirty(nsIFrame *aBFCFrame)
{
  NS_ASSERTION(aBFCFrame->GetStateBits() & NS_FRAME_FONT_INFLATION_FLOW_ROOT,
               "should have been given a flow root");

  FrameProperties bfcProps(aBFCFrame->Properties());
  nsFontInflationData *data = static_cast<nsFontInflationData*>(
                                bfcProps.Get(FontInflationDataProperty()));
  if (data) {
    data->MarkTextDirty();
  }
}

nsFontInflationData::nsFontInflationData(nsIFrame *aBFCFrame)
  : mBFCFrame(aBFCFrame)
  , mTextAmount(0)
  , mTextThreshold(0)
  , mInflationEnabled(false)
  , mTextDirty(true)
{
}








static nsIFrame*
NearestCommonAncestorFirstInFlow(nsIFrame *aFrame1, nsIFrame *aFrame2,
                                 nsIFrame *aKnownCommonAncestor)
{
  aFrame1 = aFrame1->GetFirstInFlow();
  aFrame2 = aFrame2->GetFirstInFlow();
  aKnownCommonAncestor = aKnownCommonAncestor->GetFirstInFlow();

  nsAutoTArray<nsIFrame*, 32> ancestors1, ancestors2;
  for (nsIFrame *f = aFrame1; f != aKnownCommonAncestor;
       (f = f->GetParent()) && (f = f->GetFirstInFlow())) {
    ancestors1.AppendElement(f);
  }
  for (nsIFrame *f = aFrame2; f != aKnownCommonAncestor;
       (f = f->GetParent()) && (f = f->GetFirstInFlow())) {
    ancestors2.AppendElement(f);
  }

  nsIFrame *result = aKnownCommonAncestor;
  uint32_t i1 = ancestors1.Length(),
           i2 = ancestors2.Length();
  while (i1-- != 0 && i2-- != 0) {
    if (ancestors1[i1] != ancestors2[i2]) {
      break;
    }
    result = ancestors1[i1];
  }

  return result;
}

static nscoord
ComputeDescendantWidth(const nsHTMLReflowState& aAncestorReflowState,
                       nsIFrame *aDescendantFrame)
{
  nsIFrame *ancestorFrame = aAncestorReflowState.frame->GetFirstInFlow();
  if (aDescendantFrame == ancestorFrame) {
    return aAncestorReflowState.ComputedWidth();
  }

  AutoInfallibleTArray<nsIFrame*, 16> frames;
  for (nsIFrame *f = aDescendantFrame; f != ancestorFrame;
       f = f->GetParent()->GetFirstInFlow()) {
    frames.AppendElement(f);
  }

  
  
  
  

  uint32_t len = frames.Length();
  nsHTMLReflowState *reflowStates = static_cast<nsHTMLReflowState*>
                                (moz_xmalloc(sizeof(nsHTMLReflowState) * len));
  nsPresContext *presContext = aDescendantFrame->PresContext();
  for (uint32_t i = 0; i < len; ++i) {
    const nsHTMLReflowState &parentReflowState =
      (i == 0) ? aAncestorReflowState : reflowStates[i - 1];
    nsSize availSize(parentReflowState.ComputedWidth(), NS_UNCONSTRAINEDSIZE);
    nsIFrame *frame = frames[len - i - 1];
    NS_ABORT_IF_FALSE(frame->GetParent()->GetFirstInFlow() ==
                        parentReflowState.frame->GetFirstInFlow(),
                      "bad logic in this function");
    new (reflowStates + i) nsHTMLReflowState(presContext, parentReflowState,
                                             frame, availSize);
  }

  NS_ABORT_IF_FALSE(reflowStates[len - 1].frame == aDescendantFrame,
                    "bad logic in this function");
  nscoord result = reflowStates[len - 1].ComputedWidth();

  for (uint32_t i = len; i-- != 0; ) {
    reflowStates[i].~nsHTMLReflowState();
  }
  moz_free(reflowStates);

  return result;
}

void
nsFontInflationData::UpdateWidth(const nsHTMLReflowState &aReflowState)
{
  nsIFrame *bfc = aReflowState.frame;
  NS_ASSERTION(bfc->GetStateBits() & NS_FRAME_FONT_INFLATION_FLOW_ROOT,
               "must be block formatting context");

  nsIFrame *firstInflatableDescendant =
             FindEdgeInflatableFrameIn(bfc, eFromStart);
  if (!firstInflatableDescendant) {
    mTextAmount = 0;
    mTextThreshold = 0; 
    mTextDirty = false;
    mInflationEnabled = false;
    return;
  }
  nsIFrame *lastInflatableDescendant =
             FindEdgeInflatableFrameIn(bfc, eFromEnd);
  NS_ABORT_IF_FALSE(!firstInflatableDescendant == !lastInflatableDescendant,
                    "null-ness should match; NearestCommonAncestorFirstInFlow"
                    " will crash when passed null");

  
  
  nsIFrame *nca = NearestCommonAncestorFirstInFlow(firstInflatableDescendant,
                                                   lastInflatableDescendant,
                                                   bfc);
  while (!nsLayoutUtils::IsContainerForFontSizeInflation(nca)) {
    nca = nca->GetParent()->GetFirstInFlow();
  }

  nscoord newNCAWidth = ComputeDescendantWidth(aReflowState, nca);

  
  
  nsIPresShell* presShell = bfc->PresContext()->PresShell();
  uint32_t lineThreshold = presShell->FontSizeInflationLineThreshold();
  nscoord newTextThreshold = (newNCAWidth * lineThreshold) / 100;

  if (mTextThreshold <= mTextAmount && mTextAmount < newTextThreshold) {
    
    
    mTextDirty = true;
  }

  mNCAWidth = newNCAWidth;
  mTextThreshold = newTextThreshold;
  mInflationEnabled = mTextAmount >= mTextThreshold;
}

 nsIFrame*
nsFontInflationData::FindEdgeInflatableFrameIn(nsIFrame* aFrame,
                                               SearchDirection aDirection)
{
  

  
  

  nsIFormControlFrame* fcf = do_QueryFrame(aFrame);
  if (fcf) {
    return aFrame;
  }

  
  nsAutoTArray<FrameChildList, 4> lists;
  aFrame->GetChildLists(&lists);
  for (uint32_t i = 0, len = lists.Length(); i < len; ++i) {
    const nsFrameList& list =
      lists[(aDirection == eFromStart) ? i : len - i - 1].mList;
    for (nsIFrame *kid = (aDirection == eFromStart) ? list.FirstChild()
                                                    : list.LastChild();
         kid;
         kid = (aDirection == eFromStart) ? kid->GetNextSibling()
                                          : kid->GetPrevSibling()) {
      if (kid->GetStateBits() & NS_FRAME_FONT_INFLATION_FLOW_ROOT) {
        
        continue;
      }

      if (kid->GetType() == nsGkAtoms::textFrame) {
        nsIContent *content = kid->GetContent();
        if (content && kid == content->GetPrimaryFrame()) {
          uint32_t len = nsTextFrameUtils::
            ComputeApproximateLengthWithWhitespaceCompression(
              content, kid->StyleText());
          if (len != 0) {
            return kid;
          }
        }
      } else {
        nsIFrame *kidResult =
          FindEdgeInflatableFrameIn(kid, aDirection);
        if (kidResult) {
          return kidResult;
        }
      }
    }
  }

  return nullptr;
}

void
nsFontInflationData::ScanText()
{
  mTextDirty = false;
  mTextAmount = 0;
  ScanTextIn(mBFCFrame);
  mInflationEnabled = mTextAmount >= mTextThreshold;
}

static uint32_t
DoCharCountOfLargestOption(nsIFrame *aContainer)
{
  uint32_t result = 0;
  for (nsIFrame* option = aContainer->GetFirstPrincipalChild();
       option; option = option->GetNextSibling()) {
    uint32_t optionResult;
    if (option->GetContent()->IsHTML(nsGkAtoms::optgroup)) {
      optionResult = DoCharCountOfLargestOption(option);
    } else {
      
      optionResult = 0;
      for (nsIFrame *optionChild = option->GetFirstPrincipalChild();
           optionChild; optionChild = optionChild->GetNextSibling()) {
        if (optionChild->GetType() == nsGkAtoms::textFrame) {
          optionResult += nsTextFrameUtils::
            ComputeApproximateLengthWithWhitespaceCompression(
              optionChild->GetContent(), optionChild->StyleText());
        }
      }
    }
    if (optionResult > result) {
      result = optionResult;
    }
  }
  return result;
}

static uint32_t
CharCountOfLargestOption(nsIFrame *aListControlFrame)
{
  return DoCharCountOfLargestOption(
    static_cast<nsListControlFrame*>(aListControlFrame)->GetOptionsContainer());
}

void
nsFontInflationData::ScanTextIn(nsIFrame *aFrame)
{
  

  
  

  nsIFrame::ChildListIterator lists(aFrame);
  for (; !lists.IsDone(); lists.Next()) {
    nsFrameList::Enumerator kids(lists.CurrentList());
    for (; !kids.AtEnd(); kids.Next()) {
      nsIFrame *kid = kids.get();
      if (kid->GetStateBits() & NS_FRAME_FONT_INFLATION_FLOW_ROOT) {
        
        continue;
      }

      nsIAtom *fType = kid->GetType();
      if (fType == nsGkAtoms::textFrame) {
        nsIContent *content = kid->GetContent();
        if (content && kid == content->GetPrimaryFrame()) {
          uint32_t len = nsTextFrameUtils::
            ComputeApproximateLengthWithWhitespaceCompression(
              content, kid->StyleText());
          if (len != 0) {
            nscoord fontSize = kid->StyleFont()->mFont.size;
            if (fontSize > 0) {
              mTextAmount += fontSize * len;
            }
          }
        }
      } else if (fType == nsGkAtoms::textInputFrame) {
        
        
        nscoord fontSize = kid->StyleFont()->mFont.size;
        int32_t charCount = static_cast<nsTextControlFrame*>(kid)->GetCols();
        mTextAmount += charCount * fontSize;
      } else if (fType == nsGkAtoms::comboboxControlFrame) {
        
        
        
        nscoord fontSize = kid->StyleFont()->mFont.size;
        int32_t charCount = CharCountOfLargestOption(
          static_cast<nsComboboxControlFrame*>(kid)->GetDropDown());
        mTextAmount += charCount * fontSize;
      } else if (fType == nsGkAtoms::listControlFrame) {
        
        nscoord fontSize = kid->StyleFont()->mFont.size;
        int32_t charCount = CharCountOfLargestOption(kid);
        mTextAmount += charCount * fontSize;
      } else {
        
        ScanTextIn(kid);
      }

      if (mTextAmount >= mTextThreshold) {
        return;
      }
    }
  }
}
