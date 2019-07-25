






































#ifndef GOOGLE_BREAKPAD_PROCESSOR_NETWORK_SOURCE_LINE_RESOLVER_H_
#define GOOGLE_BREAKPAD_PROCESSOR_NETWORK_SOURCE_LINE_RESOLVER_H_

#include <sys/socket.h>

#include <map>
#include <set>

#include "google_breakpad/common/breakpad_types.h"
#include "google_breakpad/processor/source_line_resolver_interface.h"
#include "google_breakpad/processor/stack_frame.h"
#include "google_breakpad/processor/symbol_supplier.h"
#include "processor/binarystream.h"
#include "processor/linked_ptr.h"
#include "processor/network_interface.h"

namespace google_breakpad {

using std::string;

class NetworkSourceLineResolver : public SourceLineResolverInterface,
                                  public SymbolSupplier {
 public:
  
  
  NetworkSourceLineResolver(const string &server,
                            unsigned short port,
                            int wait_milliseconds);
  
  NetworkSourceLineResolver(NetworkInterface *net,
                            int wait_milliseconds);
  virtual ~NetworkSourceLineResolver();
  
  
  


  
  
  
  
  virtual bool LoadModule(const CodeModule *module, const string &map_file);
  virtual bool LoadModuleUsingMapBuffer(const CodeModule *module,
                                        const string &map_buffer);

  void UnloadModule(const CodeModule *module);

  virtual bool HasModule(const CodeModule *module);

  virtual void FillSourceLineInfo(StackFrame *frame);
  virtual WindowsFrameInfo *FindWindowsFrameInfo(const StackFrame *frame);
  virtual CFIFrameInfo *FindCFIFrameInfo(const StackFrame *frame);

  
  
  
  virtual SymbolResult GetSymbolFile(const CodeModule *module,
                                     const SystemInfo *system_info,
                                     string *symbol_file);
  
  
  
  
  virtual SymbolResult GetSymbolFile(const CodeModule *module,
                                     const SystemInfo *system_info,
                                     string *symbol_file,
                                     string *symbol_data);

 private:
  int wait_milliseconds_;
  
  bool initialized_;
  
  u_int16_t sequence_;
  NetworkInterface *net_;
  
  
  
  std::set<string> module_cache_;
  
  
  std::set<string> no_symbols_cache_;

  
  
  
  
  
  typedef std::map<u_int64_t, StackFrame> SourceCache;
  SourceCache source_line_info_cache_;

  
  
  typedef std::map<u_int64_t, string> FrameInfoCache;

  typedef enum {
    kWindowsFrameInfo = 0,
    kCFIFrameInfo = 1,
  } FrameInfoType;
  FrameInfoCache frame_info_cache_[2];
  
  
  
  
  
  bool SendMessageGetResponse(const binarystream &message,
                              binarystream &response);

  
  
  bool FindCachedSourceLineInfo(StackFrame *frame) const;
  bool FindCachedFrameInfo(const StackFrame *frame,
                           FrameInfoType type,
                           string *info) const;

  
  void CacheSourceLineInfo(const StackFrame *frame);
  void CacheFrameInfo(const StackFrame *frame,
                      FrameInfoType type,
                      const string &info);

  
  NetworkSourceLineResolver(const NetworkSourceLineResolver&);
  void operator=(const NetworkSourceLineResolver&);
};

}  

#endif  
