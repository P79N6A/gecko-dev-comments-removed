









#include "nsILayoutDebugger.h"
#include "nsFrame.h"
#include "nsDisplayList.h"
#include "FrameLayerBuilder.h"
#include "nsPrintfCString.h"

#include <stdio.h>

using namespace mozilla;
using namespace mozilla::layers;

#ifdef DEBUG
class nsLayoutDebugger : public nsILayoutDebugger {
public:
  nsLayoutDebugger();
  virtual ~nsLayoutDebugger();

  NS_DECL_ISUPPORTS

  NS_IMETHOD SetShowFrameBorders(bool aEnable);

  NS_IMETHOD GetShowFrameBorders(bool* aResult);

  NS_IMETHOD SetShowEventTargetFrameBorder(bool aEnable);

  NS_IMETHOD GetShowEventTargetFrameBorder(bool* aResult);

  NS_IMETHOD GetContentSize(nsIDocument* aDocument,
                            int32_t* aSizeInBytesResult);

  NS_IMETHOD GetFrameSize(nsIPresShell* aPresentation,
                          int32_t* aSizeInBytesResult);

  NS_IMETHOD GetStyleSize(nsIPresShell* aPresentation,
                          int32_t* aSizeInBytesResult);

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
nsLayoutDebugger::SetShowFrameBorders(bool aEnable)
{
  nsFrame::ShowFrameBorders(aEnable);
  return NS_OK;
}

NS_IMETHODIMP
nsLayoutDebugger::GetShowFrameBorders(bool* aResult)
{
  *aResult = nsFrame::GetShowFrameBorders();
  return NS_OK;
}

NS_IMETHODIMP
nsLayoutDebugger::SetShowEventTargetFrameBorder(bool aEnable)
{
  nsFrame::ShowEventTargetFrameBorder(aEnable);
  return NS_OK;
}

NS_IMETHODIMP
nsLayoutDebugger::GetShowEventTargetFrameBorder(bool* aResult)
{
  *aResult = nsFrame::GetShowEventTargetFrameBorder();
  return NS_OK;
}

NS_IMETHODIMP
nsLayoutDebugger::GetContentSize(nsIDocument* aDocument,
                                 int32_t* aSizeInBytesResult)
{
  *aSizeInBytesResult = 0;
  return NS_ERROR_FAILURE;
}

NS_IMETHODIMP
nsLayoutDebugger::GetFrameSize(nsIPresShell* aPresentation,
                               int32_t* aSizeInBytesResult)
{
  *aSizeInBytesResult = 0;
  return NS_ERROR_FAILURE;
}

NS_IMETHODIMP
nsLayoutDebugger::GetStyleSize(nsIPresShell* aPresentation,
                               int32_t* aSizeInBytesResult)
{
  *aSizeInBytesResult = 0;
  return NS_ERROR_FAILURE;
}
#endif

#ifdef MOZ_DUMP_PAINTING
static int sPrintDisplayListIndent = 0;

static void
PrintDisplayListTo(nsDisplayListBuilder* aBuilder, const nsDisplayList& aList,
                   FILE* aOutput, bool aDumpHtml)
{
  if (aDumpHtml) {
    fprintf_stderr(aOutput, "<ul>");
  }

  for (nsDisplayItem* i = aList.GetBottom(); i != nullptr; i = i->GetAbove()) {
    nsCString str;
    if (aDumpHtml) {
      fprintf_stderr(aOutput, "<li>");
    } else {
      sPrintDisplayListIndent ++;
      for (int indent = 0; indent < sPrintDisplayListIndent; indent++) {
        str += "  ";
      }
    }
    nsIFrame* f = i->Frame();
    nsAutoString fName;
#ifdef DEBUG
    f->GetFrameName(fName);
#endif
    bool snap;
    nsRect rect = i->GetBounds(aBuilder, &snap);
    nscolor color;
    nsRect vis = i->GetVisibleRect();
    nsRect component = i->GetComponentAlphaBounds(aBuilder);
    nsDisplayList* list = i->GetChildren();
    const DisplayItemClip& clip = i->GetClip();
    nsRegion opaque;
#ifdef DEBUG
    if (!list || list->DidComputeVisibility()) {
      opaque = i->GetOpaqueRegion(aBuilder, &snap);
    }
#endif
    if (aDumpHtml && i->Painted()) {
      nsCString string(i->Name());
      string.Append("-");
      string.AppendInt((uint64_t)i);
      str += nsPrintfCString("<a href=\"javascript:ViewImage('%s')\">", string.BeginReading());
    }
    str += nsPrintfCString("%s %p(%s) bounds(%d,%d,%d,%d) visible(%d,%d,%d,%d) componentAlpha(%d,%d,%d,%d) clip(%s) %s",
            i->Name(), (void*)f, NS_ConvertUTF16toUTF8(fName).get(),
            rect.x, rect.y, rect.width, rect.height,
            vis.x, vis.y, vis.width, vis.height,
            component.x, component.y, component.width, component.height,
            clip.ToString().get(),
            i->IsUniform(aBuilder, &color) ? " uniform" : "");
    nsRegionRectIterator iter(opaque);
    for (const nsRect* r = iter.Next(); r; r = iter.Next()) {
      str += nsPrintfCString(" (opaque %d,%d,%d,%d)", r->x, r->y, r->width, r->height);
    }
    i->WriteDebugInfo(str);
    if (aDumpHtml && i->Painted()) {
      str += "</a>";
    }
    uint32_t key = i->GetPerFrameKey();
    Layer* layer = mozilla::FrameLayerBuilder::GetDebugOldLayerFor(f, key);
    if (layer) {
      if (aDumpHtml) {
        str += nsPrintfCString(" <a href=\"#%p\">layer=%p</a>", layer, layer);
      } else {
        str += nsPrintfCString(" layer=%p", layer);
      }
    }
    if (i->GetType() == nsDisplayItem::TYPE_SVG_EFFECTS) {
      (static_cast<nsDisplaySVGEffects*>(i))->PrintEffects(str);
    }
    fprintf_stderr(aOutput, "%s\n", str.get());
    if (list) {
      PrintDisplayListTo(aBuilder, *list, aOutput, aDumpHtml);
    }
    if (aDumpHtml) {
      fprintf_stderr(aOutput, "</li>");
    } else {
      sPrintDisplayListIndent --;
    }
  }

  if (aDumpHtml) {
    fprintf_stderr(aOutput, "</ul>");
  }
}

void
nsFrame::PrintDisplayList(nsDisplayListBuilder* aBuilder,
                          const nsDisplayList& aList,
                          FILE* aFile,
                          bool aDumpHtml)
{
  PrintDisplayListTo(aBuilder, aList, aFile, aDumpHtml);
}

static void
PrintDisplayListSetItem(nsDisplayListBuilder* aBuilder,
                        const char* aItemName,
                        const nsDisplayList& aList,
                        FILE* aFile,
                        bool aDumpHtml)
{
  if (aDumpHtml) {
    fprintf_stderr(aFile, "<li>");
  }
  fprintf_stderr(aFile, "%s", aItemName);
  PrintDisplayListTo(aBuilder, aList, aFile, aDumpHtml);
  if (aDumpHtml) {
    fprintf_stderr(aFile, "</li>");
  }
}

void
nsFrame::PrintDisplayListSet(nsDisplayListBuilder* aBuilder,
                             const nsDisplayListSet& aSet,
                             FILE *aFile,
                             bool aDumpHtml)
{
  if (aDumpHtml) {
    fprintf_stderr(aFile, "<ul>");
  }
  PrintDisplayListSetItem(aBuilder, "[BorderBackground]", *(aSet.BorderBackground()), aFile, aDumpHtml);
  PrintDisplayListSetItem(aBuilder, "[BlockBorderBackgrounds]", *(aSet.BlockBorderBackgrounds()), aFile, aDumpHtml);
  PrintDisplayListSetItem(aBuilder, "[Floats]", *(aSet.Floats()), aFile, aDumpHtml);
  PrintDisplayListSetItem(aBuilder, "[PositionedDescendants]", *(aSet.PositionedDescendants()), aFile, aDumpHtml);
  PrintDisplayListSetItem(aBuilder, "[Outlines]", *(aSet.Outlines()), aFile, aDumpHtml);
  PrintDisplayListSetItem(aBuilder, "[Content]", *(aSet.Content()), aFile, aDumpHtml);
  if (aDumpHtml) {
    fprintf_stderr(aFile, "</ul>");
  }
}

#endif
