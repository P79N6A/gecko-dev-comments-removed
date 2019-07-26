





#ifndef mozilla_plugins_MiniShmChild_h
#define mozilla_plugins_MiniShmChild_h

#include "MiniShmBase.h"

#include <string>

namespace mozilla {
namespace plugins {










class MiniShmChild : public MiniShmBase
{
public:
  MiniShmChild();
  virtual ~MiniShmChild();

  







  nsresult
  Init(MiniShmObserver* aObserver, const std::wstring& aCookie,
       const DWORD aTimeout);

  virtual nsresult
  Send() MOZ_OVERRIDE;

protected:
  void
  OnEvent() MOZ_OVERRIDE;

private:
  HANDLE mParentEvent;
  HANDLE mParentGuard;
  HANDLE mChildEvent;
  HANDLE mChildGuard;
  HANDLE mFileMapping;
  HANDLE mRegWait;
  LPVOID mView;
  DWORD  mTimeout;

  DISALLOW_COPY_AND_ASSIGN(MiniShmChild);
};

} 
} 

#endif 

