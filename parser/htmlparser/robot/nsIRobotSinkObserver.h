



































#ifndef nsIRobotSinkObserver_h___
#define nsIRobotSinkObserver_h___

#include "nsISupports.h"
class nsString;


#define NS_IROBOTSINKOBSERVER_IID \
{ 0xfab1d970, 0xcfda, 0x11d1,	  \
  {0x93, 0x28, 0x00, 0x80, 0x5f, 0x8a, 0xdd, 0x32} }

class nsIRobotSinkObserver : public nsISupports {
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_IROBOTSINKOBSERVER_IID)

  NS_IMETHOD ProcessLink(const nsString& aURLSpec) = 0;
  NS_IMETHOD VerifyDirectory(const char * verify_dir) = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIRobotSinkObserver, NS_IROBOTSINKOBSERVER_IID)

#endif 
