



#ifndef CHROME_COMMON_CHILD_PROCESS_INFO_H_
#define CHROME_COMMON_CHILD_PROCESS_INFO_H_

#include <string>

#include "base/process.h"


class ChildProcessInfo {
 public:
  enum ProcessType {
    BROWSER_PROCESS,
    RENDER_PROCESS,
    PLUGIN_PROCESS,
    WORKER_PROCESS,
    UNKNOWN_PROCESS,
  };

  
  ProcessType type() const { return type_; }

  
  
  std::wstring name() const { return name_; }

  
  base::ProcessHandle handle() const { return process_.handle(); }

  virtual int GetProcessId() const {
    if (pid_ != -1)
      return pid_;

    pid_ = process_.pid();
    return pid_;
  }
  void SetProcessBackgrounded() const { process_.SetProcessBackgrounded(true); }
  void ReduceWorkingSet() const { process_.ReduceWorkingSet(); }

  
  
  static std::wstring GetTypeNameInEnglish(ProcessType type);

  
  
  std::wstring GetLocalizedTitle() const;

  ChildProcessInfo(const ChildProcessInfo& original) {
    type_ = original.type_;
    name_ = original.name_;
    process_ = original.process_;
    pid_ = original.pid_;
  }

  ChildProcessInfo& operator=(const ChildProcessInfo& original) {
    if (&original != this) {
      type_ = original.type_;
      name_ = original.name_;
      process_ = original.process_;
      pid_ = original.pid_;
    }
    return *this;
  }

  virtual ~ChildProcessInfo();

  
  
  bool operator <(const ChildProcessInfo& rhs) const {
    if (process_.handle() != rhs.process_.handle())
      return process_ .handle() < rhs.process_.handle();
    return false;
  }

  bool operator ==(const ChildProcessInfo& rhs) const {
    return process_.handle() == rhs.process_.handle();
  }

  
  
  static std::wstring GenerateRandomChannelID(void* instance);

 protected:
  void set_type(ProcessType type) { type_ = type; }
  void set_name(const std::wstring& name) { name_ = name; }
  void set_handle(base::ProcessHandle handle) {
    process_.set_handle(handle);
    pid_ = -1;
  }

  
  ChildProcessInfo(ProcessType type);

 private:
  ProcessType type_;
  std::wstring name_;
  mutable int pid_;  

  
  mutable base::Process process_;
};

#endif  
