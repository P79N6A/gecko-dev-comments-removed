





#ifndef mozilla_plugins_MiniShmParent_h
#define mozilla_plugins_MiniShmParent_h

#include "MiniShmBase.h"

#include <string>

namespace mozilla {
namespace plugins {










class MiniShmParent : public MiniShmBase
{
public:
  MiniShmParent();
  virtual ~MiniShmParent();

  static const unsigned int kDefaultMiniShmSectionSize;

  








  nsresult
  Init(MiniShmObserver* aObserver, const DWORD aTimeout,
       const unsigned int aSectionSize = kDefaultMiniShmSectionSize);

  



  void
  CleanUp();

  






  nsresult
  GetCookie(std::wstring& aCookie);

  virtual nsresult
  Send() MOZ_OVERRIDE;

  bool
  IsConnected() const;

protected:
  void
  OnEvent() MOZ_OVERRIDE;

private:
  void
  FinalizeConnection();

  unsigned int mSectionSize;
  HANDLE mParentEvent;
  HANDLE mParentGuard;
  HANDLE mChildEvent;
  HANDLE mChildGuard;
  HANDLE mRegWait;
  HANDLE mFileMapping;
  LPVOID mView;
  bool   mIsConnected;
  DWORD  mTimeout;
  
  DISALLOW_COPY_AND_ASSIGN(MiniShmParent);
};

} 
} 

#endif 

