



#ifndef SANDBOX_WIN_SRC_BROKER_SERVICES_H_
#define SANDBOX_WIN_SRC_BROKER_SERVICES_H_

#include <list>
#include <map>
#include <set>
#include <utility>
#include "base/basictypes.h"
#include "base/compiler_specific.h"
#include "base/win/scoped_handle.h"
#include "sandbox/win/src/crosscall_server.h"
#include "sandbox/win/src/job.h"
#include "sandbox/win/src/sandbox.h"
#include "sandbox/win/src/sharedmem_ipc_server.h"
#include "sandbox/win/src/win2k_threadpool.h"
#include "sandbox/win/src/win_utils.h"

namespace {

struct JobTracker;
struct PeerTracker;

}  

namespace sandbox {

class PolicyBase;








class BrokerServicesBase final : public BrokerServices,
                                 public SingletonBase<BrokerServicesBase> {
 public:
  BrokerServicesBase();

  ~BrokerServicesBase();

  
  virtual ResultCode Init() override;
  virtual TargetPolicy* CreatePolicy() override;
  virtual ResultCode SpawnTarget(const wchar_t* exe_path,
                                 const wchar_t* command_line,
                                 TargetPolicy* policy,
                                 PROCESS_INFORMATION* target) override;
  virtual ResultCode WaitForAllTargets() override;
  virtual ResultCode AddTargetPeer(HANDLE peer_process) override;
  virtual ResultCode InstallAppContainer(const wchar_t* sid,
                                         const wchar_t* name) override;
  virtual ResultCode UninstallAppContainer(const wchar_t* sid) override;

  
  
  
  
  bool IsActiveTarget(DWORD process_id);

 private:
  
  
  static void FreeResources(JobTracker* tracker);

  
  
  static DWORD WINAPI TargetEventsThread(PVOID param);

  
  static VOID CALLBACK RemovePeer(PVOID parameter, BOOLEAN timeout);

  
  
  HANDLE job_port_;

  
  
  HANDLE no_targets_;

  
  HANDLE job_thread_;

  
  
  CRITICAL_SECTION lock_;

  
  ThreadProvider* thread_pool_;

  
  typedef std::list<JobTracker*> JobTrackerList;
  JobTrackerList tracker_list_;

  
  
  typedef std::map<DWORD, PeerTracker*> PeerTrackerMap;
  PeerTrackerMap peer_map_;

  
  
  std::set<DWORD> child_process_ids_;

  typedef std::map<uint32_t, std::pair<HANDLE, HANDLE>> TokenCacheMap;
  TokenCacheMap token_cache_;

  DISALLOW_COPY_AND_ASSIGN(BrokerServicesBase);
};

}  


#endif  
