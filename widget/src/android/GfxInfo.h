






































#ifndef __mozilla_widget_GfxInfo_h__
#define __mozilla_widget_GfxInfo_h__

#include <nsIGfxInfo.h>

#include "nsString.h"

namespace mozilla {
namespace widget {

class GfxInfo : public nsIGfxInfo
{
public:
  GfxInfo() {Init();}
  virtual ~GfxInfo() {}

  NS_DECL_ISUPPORTS
  NS_DECL_NSIGFXINFO
private:

  void Init();
  void AddCrashReportAnnotations();
  nsString mRendererIDsString;
  nsString mAdapterRAMString;

  nsString mDeviceID;
  nsString mDriverVersion;
  nsString mDriverDate;
  nsString mDeviceKey;

  PRUint32 mRendererIDs[16];
};

} 
} 

#endif 
