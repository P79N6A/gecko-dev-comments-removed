















#ifndef __ClearKeyAsyncShutdown_h__
#define __ClearKeyAsyncShutdown_h__

#include "gmp-api/gmp-async-shutdown.h"
#include "RefCounted.h"

class ClearKeyAsyncShutdown : public GMPAsyncShutdown
                            , public RefCounted
{
public:
  explicit ClearKeyAsyncShutdown(GMPAsyncShutdownHost *aHostAPI);

  void BeginShutdown() override;

private:
  virtual ~ClearKeyAsyncShutdown();

  GMPAsyncShutdownHost* mHost;
};

#endif 
