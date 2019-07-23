







































#include "nsCOMPtr.h"
#include "nsCRT.h"

#include "nsIFactory.h"
#include "nsIGenericFactory.h"
#include "nsIModule.h"
#include "nsULE.h"
#include "nsICategoryManager.h"
#include "nsEncoderDecoderUtils.h"
#include "nsUnicodeToTIS620.h"
#include "nsUnicodeToSunIndic.h"
#include "nsUnicodeToThaiTTF.h"




#define ENCODER_NAME_BASE "Unicode Encoder-"

NS_GENERIC_FACTORY_CONSTRUCTOR(nsULE)


PRInt32 g_InstanceCount = 0;
PRInt32 g_LockCount = 0;

NS_CONVERTER_REGISTRY_START
NS_UCONV_REG_UNREG_ENCODER("tis620-2", NS_UNICODETOTIS620_CID)
NS_UCONV_REG_UNREG_ENCODER("x-thaittf-0", NS_UNICODETOTHAITTF_CID)
NS_UCONV_REG_UNREG_ENCODER("x-sun-unicode-india-0", NS_UNICODETOSUNINDIC_CID)
NS_CONVERTER_REGISTRY_END

NS_IMPL_NSUCONVERTERREGSELF

NS_GENERIC_FACTORY_CONSTRUCTOR(nsUnicodeToTIS620)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsUnicodeToThaiTTF)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsUnicodeToSunIndic)

static const nsModuleComponentInfo components[] =
{
  { ENCODER_NAME_BASE "tis620-2" , NS_UNICODETOTIS620_CID,
    NS_UNICODEENCODER_CONTRACTID_BASE "tis620-2",
    nsUnicodeToTIS620Constructor,
    nsUConverterRegSelf, nsUConverterUnregSelf },
  { ENCODER_NAME_BASE "x-thaittf-0" , NS_UNICODETOTHAITTF_CID,
    NS_UNICODEENCODER_CONTRACTID_BASE "x-thaittf-0",
    nsUnicodeToThaiTTFConstructor,
    nsUConverterRegSelf, nsUConverterUnregSelf },
  { ENCODER_NAME_BASE "x-sun-unicode-india-0" , NS_UNICODETOSUNINDIC_CID,
    NS_UNICODEENCODER_CONTRACTID_BASE "x-sun-unicode-india-0",
    nsUnicodeToSunIndicConstructor,
    nsUConverterRegSelf, nsUConverterUnregSelf },
  { "Unicode Layout Engine", NS_ULE_CID, NS_ULE_PROGID, 
    nsULEConstructor, NULL, NULL }
};

NS_IMPL_NSGETMODULE(nsCtlLEModule, components)
