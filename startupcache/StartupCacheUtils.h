




































#ifndef nsStartupCacheUtils_h_
#define nsStartupCacheUtils_h_

#include "nsIStorageStream.h"
#include "nsIObjectInputStream.h"
#include "nsIObjectOutputStream.h"

namespace mozilla {
namespace scache {

NS_EXPORT nsresult
NS_NewObjectInputStreamFromBuffer(char* buffer, PRUint32 len, 
                                  nsIObjectInputStream** stream);







NS_EXPORT nsresult
NS_NewObjectOutputWrappedStorageStream(nsIObjectOutputStream **wrapperStream,
                                       nsIStorageStream** stream,
                                       PRBool wantDebugStream);

NS_EXPORT nsresult
NS_NewBufferFromStorageStream(nsIStorageStream *storageStream, 
                              char** buffer, PRUint32* len);

NS_EXPORT nsresult
NS_PathifyURI(nsIURI *in, nsACString &out);
}
}
#endif 
