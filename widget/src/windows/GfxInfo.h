






































#ifndef __mozilla_widget_GfxInfo_h__
#define __mozilla_widget_GfxInfo_h__

#include <nsIGfxInfo.h>

namespace mozilla {
namespace widget {

class GfxInfo : public nsIGfxInfo
{
public:
  GfxInfo() {}
  virtual ~GfxInfo() {}

  NS_DECL_ISUPPORTS
  NS_DECL_NSIGFXINFO

};

} 
} 

#endif
