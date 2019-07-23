




































#ifndef nsDragService_h__
#define nsDragService_h__

#include "nsBaseDragService.h"
#include "nsIDragSessionOS2.h"

#define INCL_PM
#include <os2.h>

  
class nsIURI;
class nsIURL;
class nsISupportsString;

class nsDragService : public nsBaseDragService, public nsIDragSessionOS2
{
public:
  nsDragService();
  virtual ~nsDragService();

  NS_DECL_ISUPPORTS_INHERITED

    
  NS_IMETHOD InvokeDragSession (nsIDOMNode* aDOMNode,
                                nsISupportsArray* aTransferables,
                                nsIScriptableRegion* aRegion,
                                PRUint32 aActionType);
  NS_IMETHOD StartDragSession();
  NS_IMETHOD EndDragSession(PRBool aDoneDrag);

    
  NS_IMETHOD GetNumDropItems(PRUint32* aNumDropItems);
  NS_IMETHOD GetData(nsITransferable* aTransferable, PRUint32 aItemIndex);
  NS_IMETHOD IsDataFlavorSupported(const char* aDataFlavor, PRBool* _retval);

    
  NS_IMETHOD DragOverMsg(PDRAGINFO pdinfo, MRESULT& mr, PRUint32* dragFlags);
  NS_IMETHOD GetDragoverResult(MRESULT& mr);
  NS_IMETHOD DragLeaveMsg(PDRAGINFO pdinfo, PRUint32* dragFlags);
  NS_IMETHOD DropHelpMsg(PDRAGINFO pdinfo, PRUint32* dragFlags);
  NS_IMETHOD ExitSession(PRUint32* dragFlags);
  NS_IMETHOD DropMsg(PDRAGINFO pdinfo, HWND hwnd, PRUint32* dragFlags);
  NS_IMETHOD RenderCompleteMsg(PDRAGTRANSFER pdxfer, USHORT usResult,
                               PRUint32* dragFlags);

protected:
    
  NS_IMETHOD NativeDragEnter(PDRAGINFO pdinfo);
  NS_IMETHOD NativeDrop(PDRAGINFO pdinfo, HWND hwnd, PRBool* rendering);
  NS_IMETHOD NativeRenderComplete(PDRAGTRANSFER pdxfer, USHORT usResult);
  NS_IMETHOD NativeDataToTransferable( PCSZ pszText, PCSZ pszTitle,
                                       PRBool isUrl);

  nsresult SaveAsContents(PCSZ szDest, nsIURL* aURL);
  nsresult SaveAsURL(PCSZ szDest, nsIURI* aURI);
  nsresult SaveAsText(PCSZ szDest, nsISupportsString* aString);
  nsresult GetUrlAndTitle(nsISupports* aGenericData, char** aTargetName);
  nsresult GetUniTextTitle(nsISupports* aGenericData, char** aTargetName);

  HWND     mDragWnd;
  char*    mMimeType;
  nsCOMPtr<nsISupportsArray> mSourceDataItems;
  nsCOMPtr<nsISupports>      mSourceData;

  friend MRESULT EXPENTRY nsDragWindowProc( HWND, ULONG, MPARAM, MPARAM);
};

#endif 
