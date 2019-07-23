





































#ifndef nsIKBStateControl_h__
#define nsIKBStateControl_h__

#include "nsISupports.h"


#define NS_IKBSTATECONTROL_IID \
{ 0x8c636698, 0x8075, 0x4547, \
{ 0x80, 0xad, 0xb0, 0x32, 0xf0, 0x8e, 0xf2, 0xd3 } }





class nsIKBStateControl : public nsISupports {

  public:

    NS_DECLARE_STATIC_IID_ACCESSOR(NS_IKBSTATECONTROL_IID)

    


    NS_IMETHOD ResetInputState()=0;

    









    




    NS_IMETHOD SetIMEOpenState(PRBool aState) = 0;

    




    NS_IMETHOD GetIMEOpenState(PRBool* aState) = 0;

    




    NS_IMETHOD SetIMEEnabled(PRBool aState) = 0;

    




    NS_IMETHOD GetIMEEnabled(PRBool* aState) = 0;

    


    NS_IMETHOD CancelIMEComposition() = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIKBStateControl, NS_IKBSTATECONTROL_IID)

#endif 
