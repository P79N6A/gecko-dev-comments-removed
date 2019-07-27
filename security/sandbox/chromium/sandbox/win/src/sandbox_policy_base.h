



#ifndef SANDBOX_WIN_SRC_SANDBOX_POLICY_BASE_H_
#define SANDBOX_WIN_SRC_SANDBOX_POLICY_BASE_H_

#include <windows.h>

#include <list>
#include <vector>

#include "base/basictypes.h"
#include "base/compiler_specific.h"
#include "base/strings/string16.h"
#include "sandbox/win/src/crosscall_server.h"
#include "sandbox/win/src/handle_closer.h"
#include "sandbox/win/src/ipc_tags.h"
#include "sandbox/win/src/policy_engine_opcodes.h"
#include "sandbox/win/src/policy_engine_params.h"
#include "sandbox/win/src/sandbox_policy.h"
#include "sandbox/win/src/win_utils.h"

namespace sandbox {

class AppContainerAttributes;
class LowLevelPolicy;
class TargetProcess;
struct PolicyGlobal;





class PolicyBase : public Dispatcher, public TargetPolicy {
 public:
  PolicyBase();

  
  virtual void AddRef() override;
  virtual void Release() override;
  virtual ResultCode SetTokenLevel(TokenLevel initial,
                                   TokenLevel lockdown) override;
  virtual TokenLevel GetInitialTokenLevel() const override;
  virtual TokenLevel GetLockdownTokenLevel() const override;
  virtual ResultCode SetJobLevel(JobLevel job_level,
                                 uint32 ui_exceptions) override;
  virtual ResultCode SetJobMemoryLimit(size_t memory_limit) override;
  virtual ResultCode SetAlternateDesktop(bool alternate_winstation) override;
  virtual base::string16 GetAlternateDesktop() const override;
  virtual ResultCode CreateAlternateDesktop(bool alternate_winstation) override;
  virtual void DestroyAlternateDesktop() override;
  virtual ResultCode SetIntegrityLevel(IntegrityLevel integrity_level) override;
  virtual IntegrityLevel GetIntegrityLevel() const override;
  virtual ResultCode SetDelayedIntegrityLevel(
      IntegrityLevel integrity_level) override;
  virtual ResultCode SetAppContainer(const wchar_t* sid) override;
  virtual ResultCode SetCapability(const wchar_t* sid) override;
  virtual ResultCode SetProcessMitigations(MitigationFlags flags) override;
  virtual MitigationFlags GetProcessMitigations() override;
  virtual ResultCode SetDelayedProcessMitigations(
      MitigationFlags flags) override;
  virtual MitigationFlags GetDelayedProcessMitigations() const override;
  virtual void SetStrictInterceptions() override;
  virtual ResultCode SetStdoutHandle(HANDLE handle) override;
  virtual ResultCode SetStderrHandle(HANDLE handle) override;
  virtual ResultCode AddRule(SubSystem subsystem, Semantics semantics,
                             const wchar_t* pattern) override;
  virtual ResultCode AddDllToUnload(const wchar_t* dll_name) override;
  virtual ResultCode AddKernelObjectToClose(
      const base::char16* handle_type,
      const base::char16* handle_name) override;

  
  virtual Dispatcher* OnMessageReady(IPCParams* ipc,
                                     CallbackGeneric* callback) override;
  virtual bool SetupService(InterceptionManager* manager, int service) override;

  
  
  ResultCode MakeJobObject(HANDLE* job);

  
  
  ResultCode MakeTokens(HANDLE* initial, HANDLE* lockdown);

  const AppContainerAttributes* GetAppContainer() const;

  
  
  bool AddTarget(TargetProcess* target);

  
  
  
  bool OnJobEmpty(HANDLE job);

  EvalResult EvalPolicy(int service, CountedParameterSetBase* params);

  HANDLE GetStdoutHandle();
  HANDLE GetStderrHandle();

 private:
  ~PolicyBase();

  
  bool Ping(IPCInfo* ipc, void* cookie);

  
  Dispatcher* GetDispatcher(int ipc_tag);

  
  bool SetupAllInterceptions(TargetProcess* target);

  
  bool SetupHandleCloser(TargetProcess* target);

  ResultCode AddRuleInternal(SubSystem subsystem,
                             Semantics semantics,
                             const wchar_t* pattern);

  
  CRITICAL_SECTION lock_;
  
  
  typedef std::list<TargetProcess*> TargetSet;
  TargetSet targets_;
  
  volatile LONG ref_count;
  
  TokenLevel lockdown_level_;
  TokenLevel initial_level_;
  JobLevel job_level_;
  uint32 ui_exceptions_;
  size_t memory_limit_;
  bool use_alternate_desktop_;
  bool use_alternate_winstation_;
  
  bool file_system_init_;
  bool relaxed_interceptions_;
  HANDLE stdout_handle_;
  HANDLE stderr_handle_;
  IntegrityLevel integrity_level_;
  IntegrityLevel delayed_integrity_level_;
  MitigationFlags mitigations_;
  MitigationFlags delayed_mitigations_;
  
  Dispatcher* ipc_targets_[IPC_LAST_TAG];
  
  LowLevelPolicy* policy_maker_;
  
  PolicyGlobal* policy_;
  
  std::vector<base::string16> blacklisted_dlls_;
  
  
  
  HandleCloser handle_closer_;
  std::vector<base::string16> capabilities_;
  scoped_ptr<AppContainerAttributes> appcontainer_list_;

  static HDESK alternate_desktop_handle_;
  static HWINSTA alternate_winstation_handle_;
  static IntegrityLevel alternate_desktop_integrity_level_label_;

  DISALLOW_COPY_AND_ASSIGN(PolicyBase);
};

}  

#endif  
