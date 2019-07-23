







































#ifndef nsClipboard_h__
#define nsClipboard_h__

#include <gtk/gtk.h>
#include <gtk/gtkinvisible.h>

#include "nsIClipboard.h"
#include "nsITransferable.h"
#include "nsIClipboardOwner.h"
#include <nsCOMPtr.h>

class nsITransferable;
class nsIClipboardOwner;
class nsIWidget;





class nsClipboard : public nsIClipboard
{

public:
  nsClipboard();
  virtual ~nsClipboard();

  
  NS_DECL_ISUPPORTS

  
  NS_DECL_NSICLIPBOARD

protected:
  void Init(void);

  NS_IMETHOD SetNativeClipboardData(PRInt32 aWhichClipboard);
  NS_IMETHOD GetNativeClipboardData(nsITransferable * aTransferable, 
                                    PRInt32 aWhichClipboard );

  inline nsITransferable *GetTransferable(PRInt32 aWhichClipboard);


  PRBool  mIgnoreEmptyNotification;

private:
  inline GdkAtom GetSelectionAtom(PRInt32 aWhichClipboard);
  inline void AddTarget(GdkAtom aAtom, GdkAtom aSelectionAtom);
  void RegisterFormat(const char *aMimeStr, GdkAtom aSelectionAtom);
  PRBool DoRealConvert(GdkAtom type, GdkAtom aSelectionAtom);
  PRBool DoConvert(const char *aMimeStr, GdkAtom aSelectionAtom);
  PRBool GetTargets(GdkAtom aSelectionAtom);

  nsCOMPtr<nsIClipboardOwner> mSelectionOwner;
  nsCOMPtr<nsIClipboardOwner> mGlobalOwner;
  nsCOMPtr<nsITransferable>   mSelectionTransferable;
  nsCOMPtr<nsITransferable>   mGlobalTransferable;

  
  
  GtkSelectionData mSelectionData;
  PRBool mBlocking;

  
  static GtkWidget *sWidget;

  void SelectionReceiver(GtkWidget *aWidget,
                         GtkSelectionData *aSD);

  








  static void SelectionGetCB(GtkWidget *aWidget, 
                             GtkSelectionData *aSelectionData,
                             guint      ,
                             guint      );

  






  static void SelectionClearCB(GtkWidget *aWidget, 
                               GdkEventSelection *aEvent,
                               gpointer aData);

  






  static void SelectionRequestCB(GtkWidget *aWidget,
                                 GtkSelectionData *aSelectionData,
                                 gpointer data);
  
  






  static void SelectionReceivedCB(GtkWidget *aWidget,
                                  GtkSelectionData *aSelectionData,
                                  guint aTime);


  static void SelectionNotifyCB(GtkWidget *aWidget,
                                GtkSelectionData *aSelectionData,
                                gpointer aData);
  

  
  PRBool FindSelectionNotifyEvent();


  void SetCutBuffer();
};

#endif 
