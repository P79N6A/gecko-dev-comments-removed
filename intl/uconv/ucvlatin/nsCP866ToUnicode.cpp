




#include "nsUCConstructors.h"
#include "nsCP866ToUnicode.h"
#include "mozilla/Telemetry.h"

using namespace mozilla;




nsresult
nsCP866ToUnicodeConstructor(nsISupports *aOuter, REFNSIID aIID,
                            void **aResult) 
{
  static const uint16_t g_utMappingTable[] = {
#include "cp866.ut"
  };

  Telemetry::Accumulate(Telemetry::DECODER_INSTANTIATED_IBM866, true);
  return CreateOneByteDecoder((uMappingTable*) &g_utMappingTable,
                              aOuter, aIID, aResult);
}


