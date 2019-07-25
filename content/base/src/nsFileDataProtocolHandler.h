



































#ifndef nsFileDataProtocolHandler_h___
#define nsFileDataProtocolHandler_h___

#include "nsIProtocolHandler.h"

#define FILEDATA_SCHEME "moz-filedata"

class nsIDOMFile;
class nsIPrincipal;

class nsFileDataProtocolHandler : public nsIProtocolHandler
{
public:
  NS_DECL_ISUPPORTS

  
  NS_DECL_NSIPROTOCOLHANDLER

  
  nsFileDataProtocolHandler() {}
  virtual ~nsFileDataProtocolHandler() {}

  
  static void AddFileDataEntry(nsACString& aUri,
			       nsIDOMFile* aFile,
                               nsIPrincipal* aPrincipal);
  static void RemoveFileDataEntry(nsACString& aUri);
  
};

#define NS_FILEDATAPROTOCOLHANDLER_CID \
{ 0xb43964aa, 0xa078, 0x44b2, \
  { 0xb0, 0x6b, 0xfd, 0x4d, 0x1b, 0x17, 0x2e, 0x66 } }

#endif 
