





































#include "nsIGenericFactory.h"
#include "nsDeflateConverter.h"
#include "nsZipWriter.h"

NS_GENERIC_FACTORY_CONSTRUCTOR(nsDeflateConverter)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsZipWriter)

static nsModuleComponentInfo components[] =
{
  {
    DEFLATECONVERTER_CLASSNAME,
    DEFLATECONVERTER_CID,
    DEFLATECONVERTER_CONTRACTID,
    nsDeflateConverterConstructor,
  },
  {
    ZIPWRITER_CLASSNAME,
    ZIPWRITER_CID,
    ZIPWRITER_CONTRACTID,
    nsZipWriterConstructor,
  }
};

NS_IMPL_NSGETMODULE(ZipWriterModule, components)
