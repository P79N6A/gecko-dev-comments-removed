









































#include "nsILayoutDebugger.h"
#include "nsFrame.h"
#include "nsDisplayList.h"
#include "FrameLayerBuilder.h"

#include <stdio.h>

using namespace mozilla::layers;

#ifdef NS_DEBUG
class nsLayoutDebugger : public nsILayoutDebugger {
public:
  nsLayoutDebugger();
  virtual ~nsLayoutDebugger();

  NS_DECL_ISUPPORTS

  NS_IMETHOD SetShowFrameBorders(PRBool aEnable);

  NS_IMETHOD GetShowFrameBorders(PRBool* aResult);

  NS_IMETHOD SetShowEventTargetFrameBorder(PRBool aEnable);

  NS_IMETHOD GetShowEventTargetFrameBorder(PRBool* aResult);

  NS_IMETHOD GetContentSize(nsIDocument* aDocument,
                            PRInt32* aSizeInBytesResult);

  NS_IMETHOD GetFrameSize(nsIPresShell* aPresentation,
                          PRInt32* aSizeInBytesResult);

  NS_IMETHOD GetStyleSize(nsIPresShell* aPresentation,
                          PRInt32* aSizeInBytesResult);

};

nsresult
NS_NewLayoutDebugger(nsILayoutDebugger** aResult)
{
  NS_PRECONDITION(aResult, "null OUT ptr");
  if (!aResult) {
    return NS_ERROR_NULL_POINTER;
  }
  nsLayoutDebugger* it = new nsLayoutDebugger();
  return it->QueryInterface(NS_GET_IID(nsILayoutDebugger), (void**)aResult);
}

nsLayoutDebugger::nsLayoutDebugger()
{
}

nsLayoutDebugger::~nsLayoutDebugger()
{
}

NS_IMPL_ISUPPORTS1(nsLayoutDebugger, nsILayoutDebugger)

NS_IMETHODIMP
nsLayoutDebugger::SetShowFrameBorders(PRBool aEnable)
{
  nsFrame::ShowFrameBorders(aEnable);
  return NS_OK;
}

NS_IMETHODIMP
nsLayoutDebugger::GetShowFrameBorders(PRBool* aResult)
{
  *aResult = nsFrame::GetShowFrameBorders();
  return NS_OK;
}

NS_IMETHODIMP
nsLayoutDebugger::SetShowEventTargetFrameBorder(PRBool aEnable)
{
  nsFrame::ShowEventTargetFrameBorder(aEnable);
  return NS_OK;
}

NS_IMETHODIMP
nsLayoutDebugger::GetShowEventTargetFrameBorder(PRBool* aResult)
{
  *aResult = nsFrame::GetShowEventTargetFrameBorder();
  return NS_OK;
}

NS_IMETHODIMP
nsLayoutDebugger::GetContentSize(nsIDocument* aDocument,
                                 PRInt32* aSizeInBytesResult)
{
  *aSizeInBytesResult = 0;
  return NS_ERROR_FAILURE;
}

NS_IMETHODIMP
nsLayoutDebugger::GetFrameSize(nsIPresShell* aPresentation,
                               PRInt32* aSizeInBytesResult)
{
  *aSizeInBytesResult = 0;
  return NS_ERROR_FAILURE;
}

NS_IMETHODIMP
nsLayoutDebugger::GetStyleSize(nsIPresShell* aPresentation,
                               PRInt32* aSizeInBytesResult)
{
  *aSizeInBytesResult = 0;
  return NS_ERROR_FAILURE;
}

static void
PrintDisplayListTo(nsDisplayListBuilder* aBuilder, const nsDisplayList& aList,
                   PRInt32 aIndent, FILE* aOutput)
{
  for (nsDisplayItem* i = aList.GetBottom(); i != nsnull; i = i->GetAbove()) {
    if (aList.DidComputeVisibility() && i->GetVisibleRect().IsEmpty())
      continue;
    for (PRInt32 j = 0; j < aIndent; ++j) {
      fputc(' ', aOutput);
    }
    nsIFrame* f = i->GetUnderlyingFrame();
    nsAutoString fName;
    if (f) {
      f->GetFrameName(fName);
    }
    nsRect rect = i->GetBounds(aBuilder);
    switch (i->GetType()) {
      case nsDisplayItem::TYPE_CLIP:
      case nsDisplayItem::TYPE_CLIP_ROUNDED_RECT: {
        nsDisplayClip* c = static_cast<nsDisplayClip*>(i);
        rect = c->GetClipRect();
        break;
      }
      default:
        break;
    }
    nscolor color;
    nsRect vis = i->GetVisibleRect();
    nsDisplayList* list = i->GetList();
    nsRegion opaque;
    if (!list || list->DidComputeVisibility()) {
      opaque = i->GetOpaqueRegion(aBuilder);
    }
    if (i->GetType() == nsDisplayItem::TYPE_TRANSFORM) {
      nsDisplayTransform* t = static_cast<nsDisplayTransform*>(i);
      list = t->GetStoredList()->GetList();
    }
    fprintf(aOutput, "%s %p(%s) (%d,%d,%d,%d)(%d,%d,%d,%d)%s%s",
            i->Name(), (void*)f, NS_ConvertUTF16toUTF8(fName).get(),
            rect.x, rect.y, rect.width, rect.height,
            vis.x, vis.y, vis.width, vis.height,
            opaque.IsEmpty() ? "" : " opaque",
            i->IsUniform(aBuilder, &color) ? " uniform" : "");
    if (f) {
      PRUint32 key = i->GetPerFrameKey();
      Layer* layer = aBuilder->LayerBuilder()->GetOldLayerFor(f, key);
      if (layer) {
        fprintf(aOutput, " layer=%p", layer);
      }
    }
    fputc('\n', aOutput);
    if (list) {
      PrintDisplayListTo(aBuilder, *list, aIndent + 4, aOutput);
    }
  }
}

void
nsFrame::PrintDisplayList(nsDisplayListBuilder* aBuilder,
                          const nsDisplayList& aList)
{
  PrintDisplayListTo(aBuilder, aList, 0, stdout);
}

#endif
