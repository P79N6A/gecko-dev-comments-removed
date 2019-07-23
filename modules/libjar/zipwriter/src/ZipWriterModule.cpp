





































#include "nsIGenericFactory.h"
#include "nsDeflateConverter.h"
#include "nsZipWriter.h"

NS_GENERIC_FACTORY_CONSTRUCTOR(nsDeflateConverter)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsZipWriter)

static const nsModuleComponentInfo components[] =
{
  {
    DEFLATECONVERTER_CLASSNAME,
    DEFLATECONVERTER_CID,
    "@mozilla.org/streamconv;1?from=uncompressed&to=deflate",
    nsDeflateConverterConstructor,
  },
  {
    DEFLATECONVERTER_CLASSNAME,
    DEFLATECONVERTER_CID,
    "@mozilla.org/streamconv;1?from=uncompressed&to=gzip",
    nsDeflateConverterConstructor,
  },
  {
    DEFLATECONVERTER_CLASSNAME,
    DEFLATECONVERTER_CID,
    "@mozilla.org/streamconv;1?from=uncompressed&to=x-gzip",
    nsDeflateConverterConstructor,
  },
  {
    DEFLATECONVERTER_CLASSNAME,
    DEFLATECONVERTER_CID,
    "@mozilla.org/streamconv;1?from=uncompressed&to=rawdeflate",
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
