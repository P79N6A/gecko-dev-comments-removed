




































#ifndef __nsPhMozRemoteHelper_h__
#define __nsPhMozRemoteHelper_h__

#include <nsIXRemoteWidgetHelper.h>



#define NS_PHXREMOTEWIDGETHELPER_CID \
  { 0x84f94aac, 0x1dd2, 0x11b2, \
  { 0xa0, 0x5f, 0x9b, 0x33, 0x8f, 0xea, 0x66, 0x2c } }

class nsPhXRemoteWidgetHelper : public nsIXRemoteWidgetHelper {
 public:
  nsPhXRemoteWidgetHelper();
  virtual ~nsPhXRemoteWidgetHelper();

  NS_DECL_ISUPPORTS

  NS_IMETHOD EnableXRemoteCommands( nsIWidget *aWidget, const char *aProfile, const char *aProgram );
};

#endif 
