





#ifndef mozilla_dom_cache_FileUtils_h
#define mozilla_dom_cache_FileUtils_h

#include "mozilla/Attributes.h"
#include "mozilla/dom/cache/Types.h"
#include "nsStreamUtils.h"
#include "nsTArrayForwardDeclare.h"

struct nsID;
class nsIFile;

namespace mozilla {
namespace dom {
namespace cache {

nsresult
BodyCreateDir(nsIFile* aBaseDir);




nsresult
BodyDeleteDir(nsIFile* aBaseDir);

nsresult
BodyGetCacheDir(nsIFile* aBaseDir, const nsID& aId, nsIFile** aCacheDirOut);

nsresult
BodyStartWriteStream(const QuotaInfo& aQuotaInfo, nsIFile* aBaseDir,
                     nsIInputStream* aSource, void* aClosure,
                     nsAsyncCopyCallbackFun aCallback, nsID* aIdOut,
                     nsISupports** aCopyContextOut);

void
BodyCancelWrite(nsIFile* aBaseDir, nsISupports* aCopyContext);

nsresult
BodyFinalizeWrite(nsIFile* aBaseDir, const nsID& aId);

nsresult
BodyOpen(const QuotaInfo& aQuotaInfo, nsIFile* aBaseDir, const nsID& aId,
         nsIInputStream** aStreamOut);

nsresult
BodyDeleteFiles(nsIFile* aBaseDir, const nsTArray<nsID>& aIdList);

} 
} 
} 

#endif 
