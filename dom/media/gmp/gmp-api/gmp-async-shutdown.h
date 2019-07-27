















#ifndef GMP_ASYNC_SHUTDOWN_H_
#define GMP_ASYNC_SHUTDOWN_H_

#define GMP_API_ASYNC_SHUTDOWN "async-shutdown"



















class GMPAsyncShutdown {
public:
  virtual ~GMPAsyncShutdown() {}

  virtual void BeginShutdown() = 0;
};

class GMPAsyncShutdownHost {
public:
  virtual ~GMPAsyncShutdownHost() {}

  virtual void ShutdownComplete() = 0;
};

#endif 
