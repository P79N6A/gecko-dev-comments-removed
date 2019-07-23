





































#ifndef nsIKBStateControl_h__
#define nsIKBStateControl_h__

#include "nsISupports.h"


#define NS_IKBSTATECONTROL_IID \
{ 0xac4ebf71, 0x86b6, 0x4dab, \
{ 0xa7, 0xfd, 0x17, 0x75, 0x01, 0xad, 0xec, 0x98 } }


#if defined(XP_MACOSX)





#define NS_KBSC_USE_SHARED_CONTEXT 1
#endif




class nsIKBStateControl : public nsISupports {

  public:

    NS_DECLARE_STATIC_IID_ACCESSOR(NS_IKBSTATECONTROL_IID)

    


    NS_IMETHOD ResetInputState()=0;

    









    




    NS_IMETHOD SetIMEOpenState(PRBool aState) = 0;

    




    NS_IMETHOD GetIMEOpenState(PRBool* aState) = 0;

    



    enum {
      



      IME_STATUS_DISABLED = 0,
      


      IME_STATUS_ENABLED = 1,
      





      IME_STATUS_PASSWORD = 2
    };

    


    NS_IMETHOD SetIMEEnabled(PRUint32 aState) = 0;

    


    NS_IMETHOD GetIMEEnabled(PRUint32* aState) = 0;

    


    NS_IMETHOD CancelIMEComposition() = 0;

    








    NS_IMETHOD GetToggledKeyState(PRUint32 aKeyCode, PRBool* aLEDState) = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIKBStateControl, NS_IKBSTATECONTROL_IID)

#endif 
