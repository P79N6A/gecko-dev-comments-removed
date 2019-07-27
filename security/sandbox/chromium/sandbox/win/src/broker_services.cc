



#include "sandbox/win/src/broker_services.h"

#include <AclAPI.h>

#include "base/logging.h"
#include "base/memory/scoped_ptr.h"
#include "base/threading/platform_thread.h"
#include "base/win/scoped_handle.h"
#include "base/win/scoped_process_information.h"
#include "base/win/startup_information.h"
#include "base/win/windows_version.h"
#include "sandbox/win/src/app_container.h"
#include "sandbox/win/src/process_mitigations.h"
#include "sandbox/win/src/sandbox_policy_base.h"
#include "sandbox/win/src/sandbox.h"
#include "sandbox/win/src/target_process.h"
#include "sandbox/win/src/win2k_threadpool.h"
#include "sandbox/win/src/win_utils.h"

namespace {


bool AssociateCompletionPort(HANDLE job, HANDLE port, void* key) {
  JOBOBJECT_ASSOCIATE_COMPLETION_PORT job_acp = { key, port };
  return ::SetInformationJobObject(job,
                                   JobObjectAssociateCompletionPortInformation,
                                   &job_acp, sizeof(job_acp))? true : false;
}



sandbox::ResultCode SpawnCleanup(sandbox::TargetProcess* target, DWORD error) {
  if (0 == error)
    error = ::GetLastError();

  target->Terminate();
  delete target;
  ::SetLastError(error);
  return sandbox::SBOX_ERROR_GENERIC;
}



enum {
  THREAD_CTRL_NONE,
  THREAD_CTRL_REMOVE_PEER,
  THREAD_CTRL_QUIT,
  THREAD_CTRL_LAST,
};



struct JobTracker {
  HANDLE job;
  sandbox::PolicyBase* policy;
  JobTracker(HANDLE cjob, sandbox::PolicyBase* cpolicy)
      : job(cjob), policy(cpolicy) {
  }
};


struct PeerTracker {
  HANDLE wait_object;
  base::win::ScopedHandle process;
  DWORD id;
  HANDLE job_port;
  PeerTracker(DWORD process_id, HANDLE broker_job_port)
      : wait_object(NULL), id(process_id), job_port(broker_job_port) {
  }
};

void DeregisterPeerTracker(PeerTracker* peer) {
  
  if (::UnregisterWaitEx(peer->wait_object, INVALID_HANDLE_VALUE)) {
    delete peer;
  } else {
    NOTREACHED();
  }
}



bool IsTokenCacheable(const sandbox::PolicyBase* policy) {
  const sandbox::AppContainerAttributes* app_container =
      policy->GetAppContainer();

  
  if (app_container)
    return false;

  return true;
}


uint32_t GenerateTokenCacheKey(const sandbox::PolicyBase* policy) {
  const size_t kTokenShift = 3;
  uint32_t key;

  DCHECK(IsTokenCacheable(policy));

  
  static_assert(sandbox::USER_LAST <= (1 << kTokenShift),
                "TokenLevel too large");
  static_assert(sandbox::INTEGRITY_LEVEL_LAST <= (1 << kTokenShift),
                "IntegrityLevel too large");
  static_assert(sizeof(key) < (kTokenShift * 3),
                "Token key type too small");

  
  key = policy->GetInitialTokenLevel();
  key <<= kTokenShift;
  key |= policy->GetLockdownTokenLevel();
  key <<= kTokenShift;
  key |= policy->GetIntegrityLevel();

  return key;
}

}  

namespace sandbox {

BrokerServicesBase::BrokerServicesBase()
    : thread_pool_(NULL), job_port_(NULL), no_targets_(NULL),
      job_thread_(NULL) {
}



ResultCode BrokerServicesBase::Init() {
  if ((NULL != job_port_) || (NULL != thread_pool_))
    return SBOX_ERROR_UNEXPECTED_CALL;

  ::InitializeCriticalSection(&lock_);

  job_port_ = ::CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
  if (NULL == job_port_)
    return SBOX_ERROR_GENERIC;

  no_targets_ = ::CreateEventW(NULL, TRUE, FALSE, NULL);

  job_thread_ = ::CreateThread(NULL, 0,  
                               TargetEventsThread, this, NULL, NULL);
  if (NULL == job_thread_)
    return SBOX_ERROR_GENERIC;

  return SBOX_ALL_OK;
}






BrokerServicesBase::~BrokerServicesBase() {
  
  if (!job_port_)
    return;

  
  
  
  
  ::PostQueuedCompletionStatus(job_port_, 0, THREAD_CTRL_QUIT, FALSE);
  ::CloseHandle(job_port_);

  if (WAIT_TIMEOUT == ::WaitForSingleObject(job_thread_, 1000)) {
    
    NOTREACHED();
    return;
  }

  JobTrackerList::iterator it;
  for (it = tracker_list_.begin(); it != tracker_list_.end(); ++it) {
    JobTracker* tracker = (*it);
    FreeResources(tracker);
    delete tracker;
  }
  ::CloseHandle(job_thread_);
  delete thread_pool_;
  ::CloseHandle(no_targets_);

  
  for (PeerTrackerMap::iterator it = peer_map_.begin();
       it != peer_map_.end(); ++it) {
    DeregisterPeerTracker(it->second);
  }

  
  if (job_port_)
    ::DeleteCriticalSection(&lock_);

  
  for (TokenCacheMap::iterator it = token_cache_.begin();
       it != token_cache_.end(); ++it) {
    ::CloseHandle(it->second.first);
    ::CloseHandle(it->second.second);
  }
}

TargetPolicy* BrokerServicesBase::CreatePolicy() {
  
  
  return new PolicyBase;
}

void BrokerServicesBase::FreeResources(JobTracker* tracker) {
  if (NULL != tracker->policy) {
    BOOL res = ::TerminateJobObject(tracker->job, SBOX_ALL_OK);
    DCHECK(res);
    
    
    res = ::CloseHandle(tracker->job);
    DCHECK(res);
    
    tracker->policy->OnJobEmpty(tracker->job);
    tracker->policy->Release();
    tracker->policy = NULL;
  }
}





DWORD WINAPI BrokerServicesBase::TargetEventsThread(PVOID param) {
  if (NULL == param)
    return 1;

  base::PlatformThread::SetName("BrokerEvent");

  BrokerServicesBase* broker = reinterpret_cast<BrokerServicesBase*>(param);
  HANDLE port = broker->job_port_;
  HANDLE no_targets = broker->no_targets_;

  int target_counter = 0;
  ::ResetEvent(no_targets);

  while (true) {
    DWORD events = 0;
    ULONG_PTR key = 0;
    LPOVERLAPPED ovl = NULL;

    if (!::GetQueuedCompletionStatus(port, &events, &key, &ovl, INFINITE))
      
      
      
      return 1;

    if (key > THREAD_CTRL_LAST) {
      
      
      JobTracker* tracker = reinterpret_cast<JobTracker*>(key);

      switch (events) {
        case JOB_OBJECT_MSG_ACTIVE_PROCESS_ZERO: {
          
          
          
          
          
          FreeResources(tracker);
          break;
        }

        case JOB_OBJECT_MSG_NEW_PROCESS: {
          ++target_counter;
          if (1 == target_counter) {
            ::ResetEvent(no_targets);
          }
          break;
        }

        case JOB_OBJECT_MSG_EXIT_PROCESS:
        case JOB_OBJECT_MSG_ABNORMAL_EXIT_PROCESS: {
          {
            AutoLock lock(&broker->lock_);
            broker->child_process_ids_.erase(reinterpret_cast<DWORD>(ovl));
          }
          --target_counter;
          if (0 == target_counter)
            ::SetEvent(no_targets);

          DCHECK(target_counter >= 0);
          break;
        }

        case JOB_OBJECT_MSG_ACTIVE_PROCESS_LIMIT: {
          break;
        }

        case JOB_OBJECT_MSG_PROCESS_MEMORY_LIMIT: {
          BOOL res = ::TerminateJobObject(tracker->job,
                                          SBOX_FATAL_MEMORY_EXCEEDED);
          DCHECK(res);
          break;
        }

        default: {
          NOTREACHED();
          break;
        }
      }
    } else if (THREAD_CTRL_REMOVE_PEER == key) {
      
      AutoLock lock(&broker->lock_);
      PeerTrackerMap::iterator it =
          broker->peer_map_.find(reinterpret_cast<DWORD>(ovl));
      DeregisterPeerTracker(it->second);
      broker->peer_map_.erase(it);
    } else if (THREAD_CTRL_QUIT == key) {
      
      return 0;
    } else {
      
      NOTREACHED();
    }
  }

  NOTREACHED();
  return 0;
}



ResultCode BrokerServicesBase::SpawnTarget(const wchar_t* exe_path,
                                           const wchar_t* command_line,
                                           TargetPolicy* policy,
                                           PROCESS_INFORMATION* target_info) {
  if (!exe_path)
    return SBOX_ERROR_BAD_PARAMS;

  if (!policy)
    return SBOX_ERROR_BAD_PARAMS;

  
  
  
  
  static DWORD thread_id = ::GetCurrentThreadId();
  DCHECK(thread_id == ::GetCurrentThreadId());

  AutoLock lock(&lock_);

  
  PolicyBase* policy_base = static_cast<PolicyBase*>(policy);

  
  
  HANDLE initial_token_temp;
  HANDLE lockdown_token_temp;
  ResultCode result = SBOX_ALL_OK;

  if (IsTokenCacheable(policy_base)) {
    
    
    
    uint32_t token_key = GenerateTokenCacheKey(policy_base);
    TokenCacheMap::iterator it = token_cache_.find(token_key);
    if (it != token_cache_.end()) {
      initial_token_temp = it->second.first;
      lockdown_token_temp = it->second.second;
    } else {
      result =
          policy_base->MakeTokens(&initial_token_temp, &lockdown_token_temp);
      if (SBOX_ALL_OK != result)
        return result;
      token_cache_[token_key] =
          std::pair<HANDLE, HANDLE>(initial_token_temp, lockdown_token_temp);
    }

    if (!::DuplicateToken(initial_token_temp, SecurityImpersonation,
                          &initial_token_temp)) {
      return SBOX_ERROR_GENERIC;
    }

    if (!::DuplicateTokenEx(lockdown_token_temp, TOKEN_ALL_ACCESS, 0,
                            SecurityIdentification, TokenPrimary,
                            &lockdown_token_temp)) {
      return SBOX_ERROR_GENERIC;
    }
  } else {
    result = policy_base->MakeTokens(&initial_token_temp, &lockdown_token_temp);
    if (SBOX_ALL_OK != result)
      return result;
  }

  base::win::ScopedHandle initial_token(initial_token_temp);
  base::win::ScopedHandle lockdown_token(lockdown_token_temp);

  HANDLE job_temp;
  result = policy_base->MakeJobObject(&job_temp);
  if (SBOX_ALL_OK != result)
    return result;

  base::win::ScopedHandle job(job_temp);

  
  base::win::StartupInformation startup_info;
  base::string16 desktop = policy_base->GetAlternateDesktop();
  if (!desktop.empty()) {
    startup_info.startup_info()->lpDesktop =
        const_cast<wchar_t*>(desktop.c_str());
  }

  bool inherit_handles = false;
  if (base::win::GetVersion() >= base::win::VERSION_VISTA) {
    int attribute_count = 0;
    const AppContainerAttributes* app_container =
        policy_base->GetAppContainer();
    if (app_container)
      ++attribute_count;

    DWORD64 mitigations;
    size_t mitigations_size;
    ConvertProcessMitigationsToPolicy(policy->GetProcessMitigations(),
                                      &mitigations, &mitigations_size);
    if (mitigations)
      ++attribute_count;

    HANDLE stdout_handle = policy_base->GetStdoutHandle();
    HANDLE stderr_handle = policy_base->GetStderrHandle();
    HANDLE inherit_handle_list[2];
    int inherit_handle_count = 0;
    if (stdout_handle != INVALID_HANDLE_VALUE)
      inherit_handle_list[inherit_handle_count++] = stdout_handle;
    
    if (stderr_handle != stdout_handle && stderr_handle != INVALID_HANDLE_VALUE)
      inherit_handle_list[inherit_handle_count++] = stderr_handle;
    if (inherit_handle_count)
      ++attribute_count;

    if (!startup_info.InitializeProcThreadAttributeList(attribute_count))
      return SBOX_ERROR_PROC_THREAD_ATTRIBUTES;

    if (app_container) {
      result = app_container->ShareForStartup(&startup_info);
      if (SBOX_ALL_OK != result)
        return result;
    }

    if (mitigations) {
      if (!startup_info.UpdateProcThreadAttribute(
               PROC_THREAD_ATTRIBUTE_MITIGATION_POLICY, &mitigations,
               mitigations_size)) {
        return SBOX_ERROR_PROC_THREAD_ATTRIBUTES;
      }
    }

    if (inherit_handle_count) {
      if (!startup_info.UpdateProcThreadAttribute(
              PROC_THREAD_ATTRIBUTE_HANDLE_LIST,
              inherit_handle_list,
              sizeof(inherit_handle_list[0]) * inherit_handle_count)) {
        return SBOX_ERROR_PROC_THREAD_ATTRIBUTES;
      }
      startup_info.startup_info()->dwFlags |= STARTF_USESTDHANDLES;
      startup_info.startup_info()->hStdInput = INVALID_HANDLE_VALUE;
      startup_info.startup_info()->hStdOutput = stdout_handle;
      startup_info.startup_info()->hStdError = stderr_handle;
      
      
      inherit_handles = true;
    }
  } else if (getenv("MOZ_WIN_INHERIT_STD_HANDLES_PRE_VISTA")) {
    
    
    startup_info.startup_info()->dwFlags |= STARTF_USESTDHANDLES;
    startup_info.startup_info()->hStdInput = INVALID_HANDLE_VALUE;
    startup_info.startup_info()->hStdOutput = policy_base->GetStdoutHandle();
    startup_info.startup_info()->hStdError = policy_base->GetStderrHandle();
    inherit_handles = true;
  }

  
  
  if (NULL == thread_pool_)
    thread_pool_ = new Win2kThreadPool();

  
  
  base::win::ScopedProcessInformation process_info;
  TargetProcess* target = new TargetProcess(initial_token.Take(),
                                            lockdown_token.Take(),
                                            job.Get(),
                                            thread_pool_);

  DWORD win_result = target->Create(exe_path, command_line, inherit_handles,
                                    startup_info, &process_info);
  if (ERROR_SUCCESS != win_result)
    return SpawnCleanup(target, win_result);

  
  if (!policy_base->AddTarget(target)) {
    return SpawnCleanup(target, 0);
  }

  
  
  policy_base->AddRef();
  if (job.IsValid()) {
    scoped_ptr<JobTracker> tracker(new JobTracker(job.Take(), policy_base));
    if (!AssociateCompletionPort(tracker->job, job_port_, tracker.get()))
      return SpawnCleanup(target, 0);
    
    
    tracker_list_.push_back(tracker.release());
    child_process_ids_.insert(process_info.process_id());
  } else {
    
    
    
    if (child_process_ids_.empty())
      ::SetEvent(no_targets_);
    
    
    
  }

  *target_info = process_info.Take();
  return SBOX_ALL_OK;
}


ResultCode BrokerServicesBase::WaitForAllTargets() {
  ::WaitForSingleObject(no_targets_, INFINITE);
  return SBOX_ALL_OK;
}

bool BrokerServicesBase::IsActiveTarget(DWORD process_id) {
  AutoLock lock(&lock_);
  return child_process_ids_.find(process_id) != child_process_ids_.end() ||
         peer_map_.find(process_id) != peer_map_.end();
}

VOID CALLBACK BrokerServicesBase::RemovePeer(PVOID parameter, BOOLEAN timeout) {
  PeerTracker* peer = reinterpret_cast<PeerTracker*>(parameter);
  
  ::PostQueuedCompletionStatus(peer->job_port, 0, THREAD_CTRL_REMOVE_PEER,
                               reinterpret_cast<LPOVERLAPPED>(peer->id));
}

ResultCode BrokerServicesBase::AddTargetPeer(HANDLE peer_process) {
  scoped_ptr<PeerTracker> peer(new PeerTracker(::GetProcessId(peer_process),
                                               job_port_));
  if (!peer->id)
    return SBOX_ERROR_GENERIC;

  HANDLE process_handle;
  if (!::DuplicateHandle(::GetCurrentProcess(), peer_process,
                         ::GetCurrentProcess(), &process_handle,
                         SYNCHRONIZE, FALSE, 0)) {
    return SBOX_ERROR_GENERIC;
  }
  peer->process.Set(process_handle);

  AutoLock lock(&lock_);
  if (!peer_map_.insert(std::make_pair(peer->id, peer.get())).second)
    return SBOX_ERROR_BAD_PARAMS;

  if (!::RegisterWaitForSingleObject(
          &peer->wait_object, peer->process.Get(), RemovePeer, peer.get(),
          INFINITE, WT_EXECUTEONLYONCE | WT_EXECUTEINWAITTHREAD)) {
    peer_map_.erase(peer->id);
    return SBOX_ERROR_GENERIC;
  }

  
  peer.release();
  return SBOX_ALL_OK;
}

ResultCode BrokerServicesBase::InstallAppContainer(const wchar_t* sid,
                                                   const wchar_t* name) {
  if (base::win::OSInfo::GetInstance()->version() < base::win::VERSION_WIN8)
    return SBOX_ERROR_UNSUPPORTED;

  base::string16 old_name = LookupAppContainer(sid);
  if (old_name.empty())
    return CreateAppContainer(sid, name);

  if (old_name != name)
    return SBOX_ERROR_INVALID_APP_CONTAINER;

  return SBOX_ALL_OK;
}

ResultCode BrokerServicesBase::UninstallAppContainer(const wchar_t* sid) {
  if (base::win::OSInfo::GetInstance()->version() < base::win::VERSION_WIN8)
    return SBOX_ERROR_UNSUPPORTED;

  base::string16 name = LookupAppContainer(sid);
  if (name.empty())
    return SBOX_ERROR_INVALID_APP_CONTAINER;

  return DeleteAppContainer(sid);
}

}  
