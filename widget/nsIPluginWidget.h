




































#include "nsISupports.h"

#define NS_IPLUGINWIDGET_IID    \
  { 0xEB9207E0, 0xD8F1, 0x44B9, \
    { 0xB7, 0x52, 0xAF, 0x8E, 0x9F, 0x8E, 0xBD, 0xF7 } }

struct nsIntPoint;
class nsIPluginInstanceOwner;




class NS_NO_VTABLE nsIPluginWidget : public nsISupports
{
 public: 

  NS_DECLARE_STATIC_IID_ACCESSOR(NS_IPLUGINWIDGET_IID)

  NS_IMETHOD GetPluginClipRect(nsIntRect& outClipRect, nsIntPoint& outOrigin, bool& outWidgetVisible) = 0;

  NS_IMETHOD StartDrawPlugin(void) = 0;

  NS_IMETHOD EndDrawPlugin(void) = 0;

  NS_IMETHOD SetPluginInstanceOwner(nsIPluginInstanceOwner* pluginInstanceOwner) = 0;

  NS_IMETHOD SetPluginEventModel(int inEventModel) = 0;

  NS_IMETHOD GetPluginEventModel(int* outEventModel) = 0;

  NS_IMETHOD SetPluginDrawingModel(int inDrawingModel) = 0;

  NS_IMETHOD StartComplexTextInputForCurrentEvent() = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIPluginWidget, NS_IPLUGINWIDGET_IID)
