




































#ifndef nsIMenuBar_h__
#define nsIMenuBar_h__

#include "nsISupports.h"
#include "nsString.h"
#include "nsIMenu.h"

class nsIWidget;


#define NS_IMENUBAR_IID      \
{ 0xf2e79601, 0x1700, 0x11d5, \
  { 0xbb, 0x6f, 0x90, 0xf2, 0x40, 0xfe, 0x49, 0x3c } }




class nsIMenuBar : public nsISupports {

  public:
    NS_DECLARE_STATIC_IID_ACCESSOR(NS_IMENUBAR_IID)

   



    NS_IMETHOD Create(nsIWidget * aParent) = 0;

   



    NS_IMETHOD GetParent(nsIWidget *&aParent) = 0;

   



    NS_IMETHOD SetParent(nsIWidget *aParent) = 0;

   



    NS_IMETHOD AddMenu(nsIMenu * aMenu) = 0;
    
   



    NS_IMETHOD GetMenuCount(PRUint32 &aCount) = 0;

   



    NS_IMETHOD GetMenuAt(const PRUint32 aCount, nsIMenu *& aMenu) = 0;

   



    NS_IMETHOD InsertMenuAt(const PRUint32 aCount, nsIMenu *& aMenu) = 0;

   



    NS_IMETHOD RemoveMenu(const PRUint32 aCount) = 0;

   



    NS_IMETHOD RemoveAll() = 0;

   



    NS_IMETHOD  GetNativeData(void*& aData) = 0;

   



    NS_IMETHOD  SetNativeData(void* aData) = 0;
    
   



    NS_IMETHOD  Paint() = 0;
   
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIMenuBar, NS_IMENUBAR_IID)

#endif
