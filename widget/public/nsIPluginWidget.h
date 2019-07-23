




































#include "nsISupports.h"


#define NS_IPLUGINWIDGET_IID_STR "e5576fe7-e25f-11d6-83bd-000393d7254a"

#define NS_IPLUGINWIDGET_IID \
  {0xe5576fe7, 0xe25f, 0x11d6, \
    { 0x83, 0xbd, 0x00, 0x03, 0x93, 0xd7, 0x25, 0x4a }}

struct nsRect;
struct nsPoint;

class NS_NO_VTABLE nsIPluginWidget : public nsISupports
{
 public: 

  NS_DECLARE_STATIC_IID_ACCESSOR(NS_IPLUGINWIDGET_IID)

  NS_IMETHOD GetPluginClipRect(nsRect& outClipRect, nsPoint& outOrigin, PRBool& outWidgetVisible) = 0;

  NS_IMETHOD StartDrawPlugin(void) = 0;

  NS_IMETHOD EndDrawPlugin(void) = 0;

};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIPluginWidget, NS_IPLUGINWIDGET_IID)
