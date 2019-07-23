





































#ifndef COMMON_LINUX_STABS_READER_H__
#define COMMON_LINUX_STABS_READER_H__

#include <stdint.h>
#include <cstddef>
#include <a.out.h>

#include <string>

namespace google_breakpad {

class StabsHandler;

class StabsReader {
 public:
  
  
  
  
  
  
  
  
  StabsReader(const uint8_t *stab,    size_t stab_size,
              const uint8_t *stabstr, size_t stabstr_size,
              StabsHandler *handler);

  
  
  
  
  
  bool Process();

 private:
  
  const char *SymbolString();

  
  const uint64_t SymbolValue() {
    return symbol_->n_value;
  }

  
  
  bool ProcessCompilationUnit();

  
  
  bool ProcessFunction();

  
  const struct nlist *symbols_, *symbols_end_;
  const uint8_t *stabstr_;
  size_t stabstr_size_;

  StabsHandler *handler_;

  
  const struct nlist *symbol_;

  
  const char *current_source_file_;
};







class StabsHandler {
 public:
  StabsHandler() { }
  virtual ~StabsHandler() { }

  

  
  

  
  
  
  
  
  
  
  

  
  
  
  
  
  
  
  
  
  
  

  
  
  
  
  virtual bool StartCompilationUnit(const char *filename, uint64_t address,
                                    const char *build_directory) {
    return true;
  }

  
  
  
  
  virtual bool EndCompilationUnit(uint64_t address) { return true; }

  
  
  
  
  
  
  
  
  virtual bool StartFunction(const std::string &name, uint64_t address) {
    return true;
  }

  
  
  
  
  virtual bool EndFunction(uint64_t address) { return true; }
  
  
  
  
  virtual bool Line(uint64_t address, const char *filename, int number) {
    return true;
  }

  
  
  virtual void Warning(const char *format, ...) { }
};

} 

#endif  
