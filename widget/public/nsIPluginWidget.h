




































#include "nsISupports.h"


#define NS_IPLUGINWIDGET_IID_STR "d530ce43-8f6e-45c5-a984-35c43da19073"

#define NS_IPLUGINWIDGET_IID \
  {0xd530ce43, 0x8f6e, 0x45c5, \
    { 0xa9, 0x84, 0x35, 0xc4, 0x3d, 0xa1, 0x90, 0x73 }}

struct nsIntPoint;
class nsIPluginInstanceOwner;

class NS_NO_VTABLE nsIPluginWidget : public nsISupports
{
 public: 

  NS_DECLARE_STATIC_IID_ACCESSOR(NS_IPLUGINWIDGET_IID)

  NS_IMETHOD GetPluginClipRect(nsIntRect& outClipRect, nsIntPoint& outOrigin, PRBool& outWidgetVisible) = 0;

  NS_IMETHOD StartDrawPlugin(void) = 0;

  NS_IMETHOD EndDrawPlugin(void) = 0;

  NS_IMETHOD SetPluginInstanceOwner(nsIPluginInstanceOwner* pluginInstanceOwner) = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIPluginWidget, NS_IPLUGINWIDGET_IID)
