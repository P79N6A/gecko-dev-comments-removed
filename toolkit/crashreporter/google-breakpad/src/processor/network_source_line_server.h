

































#ifndef GOOGLE_BREAKPAD_PROCESSOR_NETWORK_SOURCE_LINE_SERVER_H_
#define GOOGLE_BREAKPAD_PROCESSOR_NETWORK_SOURCE_LINE_SERVER_H_

#include <list>
#include <string>
#include <vector>

#include "google_breakpad/common/breakpad_types.h"
#include "google_breakpad/processor/basic_source_line_resolver.h"
#include "google_breakpad/processor/symbol_supplier.h"
#include "processor/binarystream.h"
#include "processor/network_interface.h"
#include "processor/udp_network.h"

namespace google_breakpad {

using std::list;
using std::string;
using std::vector;

class NetworkSourceLineServer {
 public:
  explicit NetworkSourceLineServer(SymbolSupplier *supplier,
                                   SourceLineResolverInterface *resolver,
                                   unsigned short listen_port,
                                   bool ip4only,
                                   const string &listen_address,
                                   u_int64_t max_symbol_lines)
    : initialized_(false),
      net_(new UDPNetwork(listen_address, listen_port, ip4only)),
      resolver_(resolver),
      supplier_(supplier),
      max_symbol_lines_(max_symbol_lines),
      symbol_lines_(0) {};

  NetworkSourceLineServer(SymbolSupplier *supplier,
                          SourceLineResolverInterface *resolver,
                          NetworkInterface *net,
                          u_int64_t max_symbol_lines = 0)
    : initialized_(false),
      net_(net),
      resolver_(resolver),
      supplier_(supplier),
      max_symbol_lines_(max_symbol_lines),
      symbol_lines_(0) {};

  
  
  
  bool Initialize();

  
  
  bool RunForever();

  
  
  
  bool RunOnce(int wait_milliseconds);

 private:
  bool initialized_;
  NetworkInterface *net_;
  SourceLineResolverInterface *resolver_;
  SymbolSupplier *supplier_;
  
  
  
  
  
  
  const u_int64_t max_symbol_lines_;
  
  u_int64_t symbol_lines_;
  
  list<string> modules_used_;
  
  map<string, int> module_symbol_lines_;

  void HandleHas(binarystream &message, binarystream &response);
  void HandleLoad(binarystream &message, binarystream &response);
  void HandleGet(binarystream &message, binarystream &response);
  void HandleGetStackWin(binarystream &message, binarystream &response);
  void HandleGetStackCFI(binarystream &message, binarystream &response);
  string FormatWindowsFrameInfo(WindowsFrameInfo *frame_info);

  int CountNewlines(const string &str);

  
  void UsedModule(const CodeModule &module);
  
  
  void TryUnloadModules(const CodeModule &just_loaded_module);

 protected:
  
  bool HandleRequest(binarystream &request, binarystream &response);
};

}  

#endif  
