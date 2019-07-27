





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


class FileUtils final
{
public:
  enum BodyFileType
  {
    BODY_FILE_FINAL,
    BODY_FILE_TMP
  };

  static nsresult BodyCreateDir(nsIFile* aBaseDir);
  
  
  
  static nsresult BodyDeleteDir(nsIFile* aBaseDir);
  static nsresult BodyGetCacheDir(nsIFile* aBaseDir, const nsID& aId,
                                  nsIFile** aCacheDirOut);

  static nsresult
  BodyStartWriteStream(const QuotaInfo& aQuotaInfo, nsIFile* aBaseDir,
                       nsIInputStream* aSource, void* aClosure,
                       nsAsyncCopyCallbackFun aCallback, nsID* aIdOut,
                       nsISupports** aCopyContextOut);

  static void
  BodyCancelWrite(nsIFile* aBaseDir, nsISupports* aCopyContext);

  static nsresult
  BodyFinalizeWrite(nsIFile* aBaseDir, const nsID& aId);

  static nsresult
  BodyOpen(const QuotaInfo& aQuotaInfo, nsIFile* aBaseDir, const nsID& aId,
           nsIInputStream** aStreamOut);

  static nsresult
  BodyDeleteFiles(nsIFile* aBaseDir, const nsTArray<nsID>& aIdList);

private:
  static nsresult
  BodyIdToFile(nsIFile* aBaseDir, const nsID& aId, BodyFileType aType,
               nsIFile** aBodyFileOut);

  FileUtils() = delete;
  ~FileUtils() = delete;
};

} 
} 
} 

#endif
