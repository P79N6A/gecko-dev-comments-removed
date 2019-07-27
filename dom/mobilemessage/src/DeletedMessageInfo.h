




#ifndef mozilla_dom_mobilemessage_DeletedMessageInfo_h
#define mozilla_dom_mobilemessage_DeletedMessageInfo_h

#include "mozilla/dom/mobilemessage/SmsTypes.h"
#include "nsIDeletedMessageInfo.h"

class nsIWritableVariant;

namespace mozilla {
namespace dom {
namespace mobilemessage {

class DeletedMessageInfo MOZ_FINAL : public nsIDeletedMessageInfo
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIDELETEDMESSAGEINFO

  DeletedMessageInfo(const DeletedMessageInfoData& aData);

  DeletedMessageInfo(int32_t* aMessageIds,
                     uint32_t aMsgCount,
                     uint64_t* aThreadIds,
                     uint32_t  aThreadCount);

  static nsresult Create(int32_t* aMessageIds,
                         uint32_t aMsgCount,
                         uint64_t* aThreadIds,
                         uint32_t  aThreadCount,
                         nsIDeletedMessageInfo** aDeletedInfo);

  const DeletedMessageInfoData& GetData() const { return mData; }

private:
  
  DeletedMessageInfo();

  ~DeletedMessageInfo();

  DeletedMessageInfoData mData;

  nsCOMPtr<nsIWritableVariant> mDeletedMessageIds;
  nsCOMPtr<nsIWritableVariant> mDeletedThreadIds;

protected:
  
};

} 
} 
} 

#endif
