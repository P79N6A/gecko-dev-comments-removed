



#ifndef mozilla_embedding_PrintSettingsDialogChild_h
#define mozilla_embedding_PrintSettingsDialogChild_h

#include "mozilla/embedding/PPrintSettingsDialogChild.h"
namespace mozilla {
namespace embedding {

class PrintSettingsDialogChild MOZ_FINAL : public PPrintSettingsDialogChild
{
  NS_INLINE_DECL_REFCOUNTING(PrintSettingsDialogChild)

public:
  MOZ_IMPLICIT PrintSettingsDialogChild();

  virtual bool Recv__delete__(const nsresult& aResult,
                              const PrintData& aData) MOZ_OVERRIDE;

  bool returned() { return mReturned; };
  nsresult result() { return mResult; };
  PrintData data() { return mData; };

private:
  virtual ~PrintSettingsDialogChild();
  bool mReturned;
  nsresult mResult;
  PrintData mData;
};

} 
} 

#endif
