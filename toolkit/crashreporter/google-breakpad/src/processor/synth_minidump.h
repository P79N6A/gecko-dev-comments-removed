










































































































#ifndef PROCESSOR_SYNTH_MINIDUMP_H_
#define PROCESSOR_SYNTH_MINIDUMP_H_

#include <assert.h>

#include <iostream>
#include <string>

#include "common/test_assembler.h"
#include "common/using_std_string.h"
#include "google_breakpad/common/breakpad_types.h"
#include "google_breakpad/common/minidump_format.h"

namespace google_breakpad {

namespace SynthMinidump {

using test_assembler::Endianness;
using test_assembler::kBigEndian;
using test_assembler::kLittleEndian;
using test_assembler::kUnsetEndian;
using test_assembler::Label;

class Dump;
class Memory;
class String;


class Section: public test_assembler::Section {
 public:
  explicit Section(const Dump &dump);

  
  
  
  
  
  
  
  
  void CiteLocationIn(test_assembler::Section *section) const;

  
  
  
  
  virtual void Finish(const Label &offset) { 
    file_offset_ = offset; size_ = Size();
  }

 protected:
  
  Label file_offset_, size_;
};



class Stream: public Section {
 public:
  
  
  Stream(const Dump &dump, uint32_t type) : Section(dump), type_(type) { }

  
  void CiteStreamIn(test_assembler::Section *section) const;

 private:
  
  uint32_t type_;
};

class SystemInfo: public Stream {
 public:
  
  
  
  
  
  
  
  
  SystemInfo(const Dump &dump,
             const MDRawSystemInfo &system_info,
             const String &csd_version);

  
  
  static const MDRawSystemInfo windows_x86;
  static const string windows_x86_csd_version;
};


class String: public Section {
 public:
  String(const Dump &dump, const string &value);

  
  void CiteStringIn(test_assembler::Section *section) const;
};





class Memory: public Section {
 public:
  Memory(const Dump &dump, uint64_t address)
      : Section(dump), address_(address) { start() = address; }

  
  void CiteMemoryIn(test_assembler::Section *section) const;

 private:
  
  
  uint64_t address_;
};

class Context: public Section {
 public:
  
  Context(const Dump &dump, const MDRawContextX86 &context);
  Context(const Dump &dump, const MDRawContextARM &context);
  
  Context(const Dump &dump) : Section(dump) {}
  
};

class Thread: public Section {
 public:
  
  
  Thread(const Dump &dump,
         uint32_t thread_id,
         const Memory &stack,
         const Context &context,
         uint32_t suspend_count = 0,
         uint32_t priority_class = 0,
         uint32_t priority = 0,
         uint64_t teb = 0);
};

class Module: public Section {
 public:
  
  
  
  Module(const Dump &dump,
         uint64_t base_of_image,
         uint32_t size_of_image,
         const String &name,
         uint32_t time_date_stamp = 1262805309,
         uint32_t checksum = 0,
         const MDVSFixedFileInfo &version_info = Module::stock_version_info,
         const Section *cv_record = NULL,
         const Section *misc_record = NULL);

 private:
  
  
  
  static const MDVSFixedFileInfo stock_version_info;
};

class Exception : public Stream {
public:
  Exception(const Dump &dump,
            const Context &context,
            uint32_t thread_id = 0,
            uint32_t exception_code = 0,
            uint32_t exception_flags = 0,
            uint64_t exception_address = 0);
};



template<typename Element>
class List: public Stream {
 public:
  List(const Dump &dump, uint32_t type) : Stream(dump, type), count_(0) {
    D32(count_label_);
  }

  
  void Add(Element *element) {
    element->Finish(file_offset_ + Size());
    Append(*element);
    count_++;
  }

  
  bool Empty() { return count_ == 0; }

  
  
  virtual void Finish(const Label &offset) {
    Stream::Finish(offset);
    count_label_ = count_;
  }

 private:
  size_t count_;
  Label count_label_;
};

class Dump: public test_assembler::Section {
 public:

  
  
  
  
  Dump(uint64_t flags,
       Endianness endianness = kLittleEndian,
       uint32_t version = MD_HEADER_VERSION,
       uint32_t date_time_stamp = 1262805309);

  
  
  
  
  
  Dump &Add(SynthMinidump::Section *object); 
  Dump &Add(Stream *object); 
  Dump &Add(Memory *object); 
  Dump &Add(Thread *object); 
  Dump &Add(Module *object); 

  
  
  
  
  void Finish();

 private:
  
  Label file_start_;

  
  
  SynthMinidump::Section stream_directory_; 
  size_t stream_count_;                 
  Label stream_count_label_;            
  Label stream_directory_rva_;          

  
  
  List<Thread> thread_list_;

  
  
  List<Module> module_list_;

  
  
  
  List<SynthMinidump::Section> memory_list_;
};

} 

} 

#endif  
