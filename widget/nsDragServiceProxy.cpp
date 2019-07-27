





#include "nsDragServiceProxy.h"
#include "nsIDocument.h"
#include "nsISupportsPrimitives.h"
#include "mozilla/dom/TabChild.h"
#include "mozilla/gfx/2D.h"
#include "mozilla/unused.h"
#include "nsContentUtils.h"

NS_IMPL_ISUPPORTS_INHERITED0(nsDragServiceProxy, nsBaseDragService)

nsDragServiceProxy::nsDragServiceProxy()
{
}

nsDragServiceProxy::~nsDragServiceProxy()
{
}

NS_IMETHODIMP
nsDragServiceProxy::InvokeDragSession(nsIDOMNode* aDOMNode,
                                      nsISupportsArray* aArrayTransferables,
                                      nsIScriptableRegion* aRegion,
                                      uint32_t aActionType)
{
  nsresult rv = nsBaseDragService::InvokeDragSession(aDOMNode,
                                                     aArrayTransferables,
                                                     aRegion,
                                                     aActionType);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIDOMDocument> sourceDocument;
  aDOMNode->GetOwnerDocument(getter_AddRefs(sourceDocument));
  nsCOMPtr<nsIDocument> doc = do_QueryInterface(sourceDocument);
  NS_ENSURE_STATE(doc->GetDocShell());
  mozilla::dom::TabChild* child =
    mozilla::dom::TabChild::GetFrom(doc->GetDocShell());
  NS_ENSURE_STATE(child);
  nsTArray<mozilla::dom::IPCDataTransfer> dataTransfers;
  nsContentUtils::TransferablesToIPCTransferables(aArrayTransferables,
                                                  dataTransfers,
                                                  child->Manager(),
                                                  nullptr);

  if (mHasImage || mSelection) {
    nsIntRect dragRect;
    nsPresContext* pc;
    mozilla::RefPtr<mozilla::gfx::SourceSurface> surface;
    DrawDrag(mSourceNode, aRegion, mScreenX, mScreenY,
             &dragRect, &surface, &pc);

    if (surface) {
      mozilla::RefPtr<mozilla::gfx::DataSourceSurface> dataSurface =
        surface->GetDataSurface();

      size_t length;
      int32_t stride;
      const uint8_t* data = nsContentUtils::GetSurfaceData(dataSurface, &length, &stride);
      nsDependentCString dragImage(reinterpret_cast<const char*>(data), length);

      mozilla::gfx::IntSize size = dataSurface->GetSize();
      mozilla::unused <<
        child->SendInvokeDragSession(dataTransfers, aActionType, dragImage,
                                     size.width, size.height, stride,
                                     static_cast<uint8_t>(dataSurface->GetFormat()),
                                     dragRect.x, dragRect.y);
      dataSurface->Unmap();
      StartDragSession();
      return NS_OK;
    }
  }

  mozilla::unused << child->SendInvokeDragSession(dataTransfers, aActionType,
                                                  nsCString(),
                                                  0, 0, 0, 0, 0, 0);
  StartDragSession();
  return NS_OK;
}
