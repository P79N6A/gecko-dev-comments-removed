




































#include "nsISupports.h"


#define NS_IPLUGINWIDGET_IID_STR "034E8A7E-BE36-4039-B229-39C41E9D4CD2"

#define NS_IPLUGINWIDGET_IID    \
  { 0x034E8A7E, 0xBE36, 0x4039, \
    { 0xB2, 0x29, 0x39, 0xC4, 0x1E, 0x9D, 0x4C, 0xD2 } }

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

  NS_IMETHOD SetPluginEventModel(int inEventModel) = 0;

  NS_IMETHOD GetPluginEventModel(int* outEventModel) = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIPluginWidget, NS_IPLUGINWIDGET_IID)
