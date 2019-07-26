












































#ifndef COMMON_STABS_READER_H__
#define COMMON_STABS_READER_H__

#include <stddef.h>
#include <stdint.h>

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef HAVE_A_OUT_H
#include <a.out.h>
#endif
#ifdef HAVE_MACH_O_NLIST_H
#include <mach-o/nlist.h>
#endif

#include <string>
#include <vector>

#include "common/byte_cursor.h"
#include "common/using_std_string.h"

namespace google_breakpad {

class StabsHandler;

class StabsReader {
 public:
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  StabsReader(const uint8_t *stab,    size_t stab_size,
              const uint8_t *stabstr, size_t stabstr_size,
              bool big_endian, size_t value_size, bool unitized,
              StabsHandler *handler);

  
  
  
  
  
  
  
  
  bool Process();

 private:

  
  
  
  class EntryIterator {
   public:
    
    
    struct Entry {
      
      
      bool at_end;

      
      size_t index;

      
      
      size_t name_offset;

      
      unsigned char type;
      unsigned char other;
      short descriptor;
      uint64_t value;
    };

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    EntryIterator(const ByteBuffer *buffer, bool big_endian, size_t value_size);

    
    
    EntryIterator &operator++() { Fetch(); entry_.index++; return *this; }

    
    
    
    const Entry &operator*() const { return entry_; }
    const Entry *operator->() const { return &entry_; }

   private:
    
    void Fetch();

    
    size_t value_size_;

    
    ByteCursor cursor_;

    
    Entry entry_;
  };

  
  struct Line {
    uint64_t address;
    const char *filename;
    int number;
  };

  
  const char *SymbolString();

  
  
  bool ProcessCompilationUnit();

  
  
  bool ProcessFunction();

  
  
  bool ProcessExtern();

  
  ByteBuffer entries_;

  
  ByteBuffer strings_;

  
  EntryIterator iterator_;

  
  
  bool unitized_;

  StabsHandler *handler_;

  
  size_t string_offset_;

  
  
  size_t next_cu_string_offset_;

  
  const char *current_source_file_;

  
  
  
  std::vector<Line> queued_lines_;
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

  
  
  
  
  
  
  
  
  
  
  virtual bool StartFunction(const string &name, uint64_t address) {
    return true;
  }

  
  
  
  
  virtual bool EndFunction(uint64_t address) { return true; }
  
  
  
  
  virtual bool Line(uint64_t address, const char *filename, int number) {
    return true;
  }

  
  
  virtual bool Extern(const string &name, uint64_t address) {
    return true;
  }

  
  
  virtual void Warning(const char *format, ...) = 0;
};

} 

#endif  
