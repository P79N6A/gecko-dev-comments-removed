


















#ifndef nsIDragSessionOS2_h__
#define nsIDragSessionOS2_h__

#include "nsISupports.h"

#define INCL_PM
#include <os2.h>

#define NS_IDRAGSESSIONOS2_IID_STR "bc4258b8-33ce-4624-adcb-4b62bb5164c0"
#define NS_IDRAGSESSIONOS2_IID \
  { 0xbc4258b8, 0x33ce, 0x4624, { 0xad, 0xcb, 0x4b, 0x62, 0xbb, 0x51, 0x64, 0xc0 } }

class nsIDragSessionOS2 : public nsISupports
{
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_IDRAGSESSIONOS2_IID)

  

  
  enum { DND_NONE       = 0 };                                     
  enum { DND_NATIVEDRAG = 1 };                                     
  enum { DND_MOZDRAG    = 2 };                                     
  enum { DND_INDROP     = 4 };                                     
  enum { DND_DRAGSTATUS = DND_NATIVEDRAG | DND_MOZDRAG | DND_INDROP };

  
  enum { DND_DISPATCHENTEREVENT = 16 };
  enum { DND_DISPATCHEVENT      = 32 };
  enum { DND_GETDRAGOVERRESULT  = 64 };
  enum { DND_EXITSESSION        = 128 };

  NS_IMETHOD DragOverMsg(PDRAGINFO pdinfo, MRESULT &mr, uint32_t* dragFlags) = 0;
  NS_IMETHOD GetDragoverResult(MRESULT& mr) = 0;
  NS_IMETHOD DragLeaveMsg(PDRAGINFO pdinfo, uint32_t* dragFlags) = 0;
  NS_IMETHOD DropHelpMsg(PDRAGINFO pdinfo, uint32_t* dragFlags) = 0;
  NS_IMETHOD ExitSession(uint32_t* dragFlags) = 0;
  NS_IMETHOD DropMsg(PDRAGINFO pdinfo, HWND hwnd, uint32_t* dragFlags) = 0;
  NS_IMETHOD RenderCompleteMsg(PDRAGTRANSFER pdxfer, USHORT usResult, uint32_t* dragFlags) = 0;

protected:
  NS_IMETHOD NativeDragEnter(PDRAGINFO pdinfo) = 0;
  NS_IMETHOD NativeDrop(PDRAGINFO pdinfo, HWND hwnd, bool* rendering) = 0;
  NS_IMETHOD NativeRenderComplete(PDRAGTRANSFER pdxfer, USHORT usResult) = 0;
  NS_IMETHOD NativeDataToTransferable( PCSZ pszText, PCSZ pszTitle, bool isUrl) = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIDragSessionOS2, NS_IDRAGSESSIONOS2_IID)

#endif  

