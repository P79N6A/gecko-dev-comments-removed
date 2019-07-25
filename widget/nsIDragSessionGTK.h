






































#ifndef nsIDragSessionGTK_h_
#define nsIDragSessionGTK_h_

#include "nsISupports.h"

#include <gtk/gtk.h>

typedef void (*nsIDragSessionGTKTimeCB)(guint32 *aTime);

#define NS_IDRAGSESSIONGTK_IID \
{ 0xa6b49c42, 0x1dd1, 0x11b2, { 0xb2, 0xdf, 0xc1, 0xd6, 0x1d, 0x67, 0x45, 0xcf } }

class nsIDragSessionGTK : public nsISupports {
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_IDRAGSESSIONGTK_IID)

  
  
  
    
  
  
  NS_IMETHOD TargetSetLastContext  (GtkWidget      *aWidget,
                                    GdkDragContext *aContext,
                                    guint           aTime) = 0;
  
  NS_IMETHOD TargetStartDragMotion (void) = 0;
  
  NS_IMETHOD TargetEndDragMotion   (GtkWidget      *aWidget,
                                    GdkDragContext *aContext,
                                    guint           aTime) = 0;
  
  NS_IMETHOD TargetDataReceived    (GtkWidget         *aWidget,
                                    GdkDragContext    *aContext,
                                    gint               aX,
                                    gint               aY,
                                    GtkSelectionData  *aSelection_data,
                                    guint              aInfo,
                                    guint32            aTime) = 0;
  
  NS_IMETHOD TargetSetTimeCallback (nsIDragSessionGTKTimeCB aCallback) = 0;

};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIDragSessionGTK, NS_IDRAGSESSIONGTK_IID)

#endif 
