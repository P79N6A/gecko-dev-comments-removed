



#ifndef SANDBOX_SRC_CROSSCALL_SERVER_H_
#define SANDBOX_SRC_CROSSCALL_SERVER_H_

#include <string>
#include <vector>
#include "base/basictypes.h"
#include "base/callback.h"
#include "base/strings/string16.h"
#include "sandbox/win/src/crosscall_params.h"






























namespace sandbox {

class InterceptionManager;





typedef void (__stdcall * CrossCallIPCCallback)(void* context,
                                                unsigned char reason);












class ThreadProvider {
 public:
  
  
  
  
  
  
  
  
  
  virtual bool RegisterWait(const void* client, HANDLE waitable_object,
                            CrossCallIPCCallback callback,
                            void* context) = 0;

  
  
  virtual bool UnRegisterWaits(void* cookie) = 0;
  virtual ~ThreadProvider() {}
};




class CrossCallParamsEx : public CrossCallParams {
 public:
  
  
  
  
  static CrossCallParamsEx* CreateFromBuffer(void* buffer_base,
                                             uint32 buffer_size,
                                             uint32* output_size);

  
  
  
  
  void* GetRawParameter(uint32 index, uint32* size, ArgType* type);

  
  
  bool GetParameter32(uint32 index, uint32* param);

  
  
  bool GetParameterVoidPtr(uint32 index, void** param);

  
  
  bool GetParameterStr(uint32 index, base::string16* string);

  
  
  
  bool GetParameterPtr(uint32 index, uint32 expected_size, void** pointer);

  
  static void operator delete(void* raw_memory) throw();

 private:
  
  CrossCallParamsEx();

  ParamInfo param_info_[1];
  DISALLOW_COPY_AND_ASSIGN(CrossCallParamsEx);
};



void SetCallError(ResultCode error, CrossCallReturn* call_return);



void SetCallSuccess(CrossCallReturn* call_return);



struct ClientInfo {
  HANDLE process;
  HANDLE job_object;
  DWORD process_id;
};


struct IPCInfo {
  int ipc_tag;
  const ClientInfo* client_info;
  CrossCallReturn return_info;
};


struct IPCParams {
  int ipc_tag;
  ArgType args[kMaxIpcParams];

  bool Matches(IPCParams* other) const {
    return !memcmp(this, other, sizeof(*other));
  }
};









class Dispatcher {
 public:
  
  typedef bool (Dispatcher::*CallbackGeneric)();
  typedef bool (Dispatcher::*Callback0)(IPCInfo* ipc);
  typedef bool (Dispatcher::*Callback1)(IPCInfo* ipc, void* p1);
  typedef bool (Dispatcher::*Callback2)(IPCInfo* ipc, void* p1, void* p2);
  typedef bool (Dispatcher::*Callback3)(IPCInfo* ipc, void* p1, void* p2,
                                        void* p3);
  typedef bool (Dispatcher::*Callback4)(IPCInfo* ipc, void* p1, void* p2,
                                        void* p3, void* p4);
  typedef bool (Dispatcher::*Callback5)(IPCInfo* ipc, void* p1, void* p2,
                                        void* p3, void* p4, void* p5);
  typedef bool (Dispatcher::*Callback6)(IPCInfo* ipc, void* p1, void* p2,
                                        void* p3, void* p4, void* p5, void* p6);
  typedef bool (Dispatcher::*Callback7)(IPCInfo* ipc, void* p1, void* p2,
                                        void* p3, void* p4, void* p5, void* p6,
                                        void* p7);
  typedef bool (Dispatcher::*Callback8)(IPCInfo* ipc, void* p1, void* p2,
                                        void* p3, void* p4, void* p5, void* p6,
                                        void* p7, void* p8);
  typedef bool (Dispatcher::*Callback9)(IPCInfo* ipc, void* p1, void* p2,
                                        void* p3, void* p4, void* p5, void* p6,
                                        void* p7, void* p8, void* p9);

  
  
  
  
  virtual Dispatcher* OnMessageReady(IPCParams* ipc, CallbackGeneric* callback);

  
  
  virtual bool SetupService(InterceptionManager* manager, int service) = 0;

  virtual ~Dispatcher() {}

 protected:
  
  struct IPCCall {
    IPCParams params;
    CallbackGeneric callback;
  };

  
  std::vector<IPCCall> ipc_calls_;
};

}  

#endif  
