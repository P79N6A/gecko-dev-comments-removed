



































#ifndef __nsInternetConfigService_h
#define __nsInternetConfigService_h

#include "nsIInternetConfigService.h"
#include "nsInternetConfig.h"

#define NS_INTERNETCONFIGSERVICE_CID \
  {0x9b8b9d81, 0x5f4f, 0x11d4, \
    { 0x96, 0x96, 0x00, 0x60, 0x08, 0x3a, 0x0b, 0xcf }}

class nsInternetConfigService : public nsIInternetConfigService
{
public:

  NS_DECL_ISUPPORTS
  NS_DECL_NSIINTERNETCONFIGSERVICE

  nsresult GetMappingForMIMEType(const char *mimetype, const char *fileextension, ICMapEntry *entry);

  nsInternetConfigService();
  virtual ~nsInternetConfigService();
protected:
  
  nsresult FillMIMEInfoForICEntry(ICMapEntry& entry, nsIMIMEInfo ** mimeinfo);
  
  nsresult GetICKeyPascalString(PRUint32 inIndex, const unsigned char*& outICKey);
  nsresult GetICPreference(PRUint32 inKey, void *outData, long *ioSize);
};

#endif
