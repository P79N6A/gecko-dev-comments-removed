











































#ifndef nsIFormSubmitObserver_h__
#define nsIFormSubmitObserver_h__

#include "nsISupports.h"
#include "prtypes.h"

class nsIContent;
class nsIDOMWindowInternal;
class nsIURI;


#define NS_IFORMSUBMITOBSERVER_IID      \
{ 0xa6cf9106, 0x15b3, 0x11d2, {0x93, 0x2e, 0x00, 0x80, 0x5f, 0x8a, 0xdd, 0x32} }

#define NS_FORMSUBMIT_SUBJECT "formsubmit"

#define NS_EARLYFORMSUBMIT_SUBJECT "earlyformsubmit"
#define NS_FIRST_FORMSUBMIT_CATEGORY "firstformsubmit"
#define NS_PASSWORDMANAGER_CATEGORY "passwordmanager"

class nsIFormSubmitObserver : public nsISupports {
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_IFORMSUBMITOBSERVER_IID)

  








  NS_IMETHOD Notify(nsIContent* formNode, nsIDOMWindowInternal* window, nsIURI* actionURL, PRBool* cancelSubmit) = 0;

};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIFormSubmitObserver,
                              NS_IFORMSUBMITOBSERVER_IID)

#endif 

