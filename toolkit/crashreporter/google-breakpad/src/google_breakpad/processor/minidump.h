













































































#ifndef GOOGLE_BREAKPAD_PROCESSOR_MINIDUMP_H__
#define GOOGLE_BREAKPAD_PROCESSOR_MINIDUMP_H__

#ifndef _WIN32
#include <unistd.h>
#endif

#include <iostream>
#include <map>
#include <string>
#include <vector>

#include "common/using_std_string.h"
#include "google_breakpad/common/minidump_format.h"
#include "google_breakpad/processor/code_module.h"
#include "google_breakpad/processor/code_modules.h"
#include "google_breakpad/processor/memory_region.h"


namespace google_breakpad {


using std::map;
using std::vector;


class Minidump;
template<typename AddressType, typename EntryType> class RangeMap;




class MinidumpObject {
 public:
  virtual ~MinidumpObject() {}

  bool valid() const { return valid_; }

 protected:
  explicit MinidumpObject(Minidump* minidump);

  
  
  
  
  
  
  Minidump* minidump_;

  
  
  
  
  bool      valid_;
};








class MinidumpStream : public MinidumpObject {
 public:
  virtual ~MinidumpStream() {}

 protected:
  explicit MinidumpStream(Minidump* minidump);

 private:
  
  
  
  
  
  
  
  virtual bool Read(uint32_t expected_size) = 0;
};












class MinidumpContext : public MinidumpStream {
 public:
  virtual ~MinidumpContext();

  
  
  
  
  uint32_t GetContextCPU() const;

  
  
  bool GetInstructionPointer(uint64_t* ip) const;

  
  
  
  const MDRawContextAMD64* GetContextAMD64() const;
  const MDRawContextARM*   GetContextARM() const;
  const MDRawContextPPC*   GetContextPPC() const;
  const MDRawContextSPARC* GetContextSPARC() const;
  const MDRawContextX86*   GetContextX86() const;

  
  void Print();

 protected:
  explicit MinidumpContext(Minidump* minidump);

  
  union {
    MDRawContextBase*  base;
    MDRawContextX86*   x86;
    MDRawContextPPC*   ppc;
    MDRawContextAMD64* amd64;
    
    
    MDRawContextSPARC* ctx_sparc;
    MDRawContextARM*   arm;
  } context_;

  
  uint32_t context_flags_;

 private:
  friend class MinidumpThread;
  friend class MinidumpException;

  bool Read(uint32_t expected_size);

  
  void FreeContext();

  
  
  
  
  
  bool CheckAgainstSystemInfo(uint32_t context_cpu_type);
};










class MinidumpMemoryRegion : public MinidumpObject,
                             public MemoryRegion {
 public:
  virtual ~MinidumpMemoryRegion();

  static void set_max_bytes(uint32_t max_bytes) { max_bytes_ = max_bytes; }
  static uint32_t max_bytes() { return max_bytes_; }

  
  
  
  const uint8_t* GetMemory() const;

  
  uint64_t GetBase() const;

  
  uint32_t GetSize() const;

  
  void FreeMemory();

  
  bool GetMemoryAtAddress(uint64_t address, uint8_t*  value) const;
  bool GetMemoryAtAddress(uint64_t address, uint16_t* value) const;
  bool GetMemoryAtAddress(uint64_t address, uint32_t* value) const;
  bool GetMemoryAtAddress(uint64_t address, uint64_t* value) const;

  
  void Print();

 protected:
  explicit MinidumpMemoryRegion(Minidump* minidump);

 private:
  friend class MinidumpThread;
  friend class MinidumpMemoryList;

  
  
  void SetDescriptor(MDMemoryDescriptor* descriptor);

  
  template<typename T> bool GetMemoryAtAddressInternal(uint64_t address,
                                                       T*        value) const;

  
  
  static uint32_t max_bytes_;

  
  
  MDMemoryDescriptor* descriptor_;

  
  mutable vector<uint8_t>* memory_;
};









class MinidumpThread : public MinidumpObject {
 public:
  virtual ~MinidumpThread();

  const MDRawThread* thread() const { return valid_ ? &thread_ : NULL; }
  
  
  virtual MinidumpMemoryRegion* GetMemory();
  
  virtual MinidumpContext* GetContext();

  
  
  
  
  virtual bool GetThreadID(uint32_t *thread_id) const;

  
  void Print();

 protected:
  explicit MinidumpThread(Minidump* minidump);

 private:
  
  friend class MinidumpThreadList;

  
  
  
  bool Read();

  MDRawThread           thread_;
  MinidumpMemoryRegion* memory_;
  MinidumpContext*      context_;
};




class MinidumpThreadList : public MinidumpStream {
 public:
  virtual ~MinidumpThreadList();

  static void set_max_threads(uint32_t max_threads) {
    max_threads_ = max_threads;
  }
  static uint32_t max_threads() { return max_threads_; }

  virtual unsigned int thread_count() const {
    return valid_ ? thread_count_ : 0;
  }

  
  virtual MinidumpThread* GetThreadAtIndex(unsigned int index) const;

  
  MinidumpThread* GetThreadByID(uint32_t thread_id);

  
  void Print();

 protected:
  explicit MinidumpThreadList(Minidump* aMinidump);

 private:
  friend class Minidump;

  typedef map<uint32_t, MinidumpThread*> IDToThreadMap;
  typedef vector<MinidumpThread> MinidumpThreads;

  static const uint32_t kStreamType = MD_THREAD_LIST_STREAM;

  bool Read(uint32_t aExpectedSize);

  
  
  static uint32_t max_threads_;

  
  IDToThreadMap    id_to_thread_map_;

  
  MinidumpThreads* threads_;
  uint32_t        thread_count_;
};






class MinidumpModule : public MinidumpObject,
                       public CodeModule {
 public:
  virtual ~MinidumpModule();

  static void set_max_cv_bytes(uint32_t max_cv_bytes) {
    max_cv_bytes_ = max_cv_bytes;
  }
  static uint32_t max_cv_bytes() { return max_cv_bytes_; }

  static void set_max_misc_bytes(uint32_t max_misc_bytes) {
    max_misc_bytes_ = max_misc_bytes;
  }
  static uint32_t max_misc_bytes() { return max_misc_bytes_; }

  const MDRawModule* module() const { return valid_ ? &module_ : NULL; }

  
  virtual uint64_t base_address() const {
    return valid_ ? module_.base_of_image : static_cast<uint64_t>(-1);
  }
  virtual uint64_t size() const { return valid_ ? module_.size_of_image : 0; }
  virtual string code_file() const;
  virtual string code_identifier() const;
  virtual string debug_file() const;
  virtual string debug_identifier() const;
  virtual string version() const;
  virtual const CodeModule* Copy() const;

  
  
  
  
  
  
  
  
  
  
  const uint8_t* GetCVRecord(uint32_t* size);

  
  
  
  
  
  const MDImageDebugMisc* GetMiscRecord(uint32_t* size);

  
  void Print();

 private:
  
  friend class MinidumpModuleList;

  explicit MinidumpModule(Minidump* minidump);

  
  
  
  bool Read();

  
  
  
  
  
  
  bool ReadAuxiliaryData();

  
  
  
  static uint32_t max_cv_bytes_;
  static uint32_t max_misc_bytes_;

  
  
  
  
  
  bool              module_valid_;

  
  
  
  bool              has_debug_info_;

  MDRawModule       module_;

  
  const string*     name_;

  
  
  
  
  vector<uint8_t>* cv_record_;

  
  
  
  uint32_t cv_record_signature_;

  
  
  
  vector<uint8_t>* misc_record_;
};






class MinidumpModuleList : public MinidumpStream,
                           public CodeModules {
 public:
  virtual ~MinidumpModuleList();

  static void set_max_modules(uint32_t max_modules) {
    max_modules_ = max_modules;
  }
  static uint32_t max_modules() { return max_modules_; }

  
  virtual unsigned int module_count() const {
    return valid_ ? module_count_ : 0;
  }
  virtual const MinidumpModule* GetModuleForAddress(uint64_t address) const;
  virtual const MinidumpModule* GetMainModule() const;
  virtual const MinidumpModule* GetModuleAtSequence(
      unsigned int sequence) const;
  virtual const MinidumpModule* GetModuleAtIndex(unsigned int index) const;
  virtual const CodeModules* Copy() const;

  
  void Print();

 protected:
  explicit MinidumpModuleList(Minidump* minidump);

 private:
  friend class Minidump;

  typedef vector<MinidumpModule> MinidumpModules;

  static const uint32_t kStreamType = MD_MODULE_LIST_STREAM;

  bool Read(uint32_t expected_size);

  
  
  static uint32_t max_modules_;

  
  RangeMap<uint64_t, unsigned int> *range_map_;

  MinidumpModules *modules_;
  uint32_t module_count_;
};











class MinidumpMemoryList : public MinidumpStream {
 public:
  virtual ~MinidumpMemoryList();

  static void set_max_regions(uint32_t max_regions) {
    max_regions_ = max_regions;
  }
  static uint32_t max_regions() { return max_regions_; }

  unsigned int region_count() const { return valid_ ? region_count_ : 0; }

  
  MinidumpMemoryRegion* GetMemoryRegionAtIndex(unsigned int index);

  
  
  MinidumpMemoryRegion* GetMemoryRegionForAddress(uint64_t address);

  
  void Print();

 private:
  friend class Minidump;

  typedef vector<MDMemoryDescriptor>   MemoryDescriptors;
  typedef vector<MinidumpMemoryRegion> MemoryRegions;

  static const uint32_t kStreamType = MD_MEMORY_LIST_STREAM;

  explicit MinidumpMemoryList(Minidump* minidump);

  bool Read(uint32_t expected_size);

  
  
  static uint32_t max_regions_;

  
  RangeMap<uint64_t, unsigned int> *range_map_;

  
  
  
  
  MemoryDescriptors *descriptors_;

  
  MemoryRegions *regions_;
  uint32_t region_count_;
};








class MinidumpException : public MinidumpStream {
 public:
  virtual ~MinidumpException();

  const MDRawExceptionStream* exception() const {
    return valid_ ? &exception_ : NULL;
  }

  
  
  
  
  bool GetThreadID(uint32_t *thread_id) const;

  MinidumpContext* GetContext();

  
  void Print();

 private:
  friend class Minidump;

  static const uint32_t kStreamType = MD_EXCEPTION_STREAM;

  explicit MinidumpException(Minidump* minidump);

  bool Read(uint32_t expected_size);

  MDRawExceptionStream exception_;
  MinidumpContext*     context_;
};



class MinidumpAssertion : public MinidumpStream {
 public:
  virtual ~MinidumpAssertion();

  const MDRawAssertionInfo* assertion() const {
    return valid_ ? &assertion_ : NULL;
  }

  string expression() const {
    return valid_ ? expression_ : "";
  }

  string function() const {
    return valid_ ? function_ : "";
  }

  string file() const {
    return valid_ ? file_ : "";
  }

  
  void Print();

 private:
  friend class Minidump;

  static const uint32_t kStreamType = MD_ASSERTION_INFO_STREAM;

  explicit MinidumpAssertion(Minidump* minidump);

  bool Read(uint32_t expected_size);

  MDRawAssertionInfo assertion_;
  string expression_;
  string function_;
  string file_;
};




class MinidumpSystemInfo : public MinidumpStream {
 public:
  virtual ~MinidumpSystemInfo();

  const MDRawSystemInfo* system_info() const {
    return valid_ ? &system_info_ : NULL;
  }

  
  
  
  
  
  
  string GetOS();
  string GetCPU();

  
  
  
  
  const string* GetCSDVersion();

  
  
  
  const string* GetCPUVendor();

  
  void Print();

 protected:
  explicit MinidumpSystemInfo(Minidump* minidump);
  MDRawSystemInfo system_info_;

  
  
  const string* csd_version_;

 private:
  friend class Minidump;

  static const uint32_t kStreamType = MD_SYSTEM_INFO_STREAM;

  bool Read(uint32_t expected_size);

  
  const string* cpu_vendor_;
};





class MinidumpMiscInfo : public MinidumpStream {
 public:
  const MDRawMiscInfo* misc_info() const {
    return valid_ ? &misc_info_ : NULL;
  }

  
  void Print();

 private:
  friend class Minidump;

  static const uint32_t kStreamType = MD_MISC_INFO_STREAM;

  explicit MinidumpMiscInfo(Minidump* minidump_);

  bool Read(uint32_t expected_size_);

  MDRawMiscInfo misc_info_;
};





class MinidumpBreakpadInfo : public MinidumpStream {
 public:
  const MDRawBreakpadInfo* breakpad_info() const {
    return valid_ ? &breakpad_info_ : NULL;
  }

  
  
  
  
  bool GetDumpThreadID(uint32_t *thread_id) const;
  bool GetRequestingThreadID(uint32_t *thread_id) const;

  
  void Print();

 private:
  friend class Minidump;

  static const uint32_t kStreamType = MD_BREAKPAD_INFO_STREAM;

  explicit MinidumpBreakpadInfo(Minidump* minidump_);

  bool Read(uint32_t expected_size_);

  MDRawBreakpadInfo breakpad_info_;
};




class MinidumpMemoryInfo : public MinidumpObject {
 public:
  const MDRawMemoryInfo* info() const { return valid_ ? &memory_info_ : NULL; }

  
  uint64_t GetBase() const { return valid_ ? memory_info_.base_address : 0; }

  
  uint32_t GetSize() const { return valid_ ? memory_info_.region_size : 0; }

  
  bool IsExecutable() const;

  
  bool IsWritable() const;

  
  void Print();

 private:
  
  friend class MinidumpMemoryInfoList;

  explicit MinidumpMemoryInfo(Minidump* minidump);

  
  
  
  bool Read();

  MDRawMemoryInfo memory_info_;
};





class MinidumpMemoryInfoList : public MinidumpStream {
 public:
  virtual ~MinidumpMemoryInfoList();

  unsigned int info_count() const { return valid_ ? info_count_ : 0; }

  const MinidumpMemoryInfo* GetMemoryInfoForAddress(uint64_t address) const;
  const MinidumpMemoryInfo* GetMemoryInfoAtIndex(unsigned int index) const;

  
  void Print();

 private:
  friend class Minidump;

  typedef vector<MinidumpMemoryInfo> MinidumpMemoryInfos;

  static const uint32_t kStreamType = MD_MEMORY_INFO_LIST_STREAM;

  explicit MinidumpMemoryInfoList(Minidump* minidump);

  bool Read(uint32_t expected_size);

  
  RangeMap<uint64_t, unsigned int> *range_map_;

  MinidumpMemoryInfos* infos_;
  uint32_t info_count_;
};




class Minidump {
 public:
  
  explicit Minidump(const string& path);
  
  
  
  explicit Minidump(std::istream& input);

  virtual ~Minidump();

  
  virtual string path() const {
    return path_;
  }
  static void set_max_streams(uint32_t max_streams) {
    max_streams_ = max_streams;
  }
  static uint32_t max_streams() { return max_streams_; }

  static void set_max_string_length(uint32_t max_string_length) {
    max_string_length_ = max_string_length;
  }
  static uint32_t max_string_length() { return max_string_length_; }

  virtual const MDRawHeader* header() const { return valid_ ? &header_ : NULL; }

  
  
  
  
  
  
  
  
  bool GetContextCPUFlagsFromSystemInfo(uint32_t* context_cpu_flags);

  
  
  
  
  virtual bool Read();

  
  
  
  
  virtual MinidumpThreadList* GetThreadList();
  MinidumpModuleList* GetModuleList();
  MinidumpMemoryList* GetMemoryList();
  MinidumpException* GetException();
  MinidumpAssertion* GetAssertion();
  virtual MinidumpSystemInfo* GetSystemInfo();
  MinidumpMiscInfo* GetMiscInfo();
  MinidumpBreakpadInfo* GetBreakpadInfo();
  MinidumpMemoryInfoList* GetMemoryInfoList();

  
  
  
  

  unsigned int GetDirectoryEntryCount() const {
    return valid_ ? header_.stream_count : 0;
  }
  const MDRawDirectory* GetDirectoryEntryAtIndex(unsigned int index) const;

  

  
  
  
  bool ReadBytes(void* bytes, size_t count);

  
  bool SeekSet(off_t offset);

  
  off_t Tell();

  

  
  
  
  string* ReadString(off_t offset);

  
  
  
  
  
  
  
  
  
  
  
  
  bool SeekToStreamType(uint32_t stream_type, uint32_t* stream_length);

  bool swap() const { return valid_ ? swap_ : false; }

  
  void Print();

 private:
  
  
  
  struct MinidumpStreamInfo {
    MinidumpStreamInfo() : stream_index(0), stream(NULL) {}
    ~MinidumpStreamInfo() { delete stream; }

    
    unsigned int    stream_index;

    
    MinidumpStream* stream;
  };

  typedef vector<MDRawDirectory> MinidumpDirectoryEntries;
  typedef map<uint32_t, MinidumpStreamInfo> MinidumpStreamMap;

  template<typename T> T* GetStream(T** stream);

  
  bool Open();

  
  
  
  static uint32_t max_streams_;

  
  
  
  
  static unsigned int max_string_length_;

  MDRawHeader               header_;

  
  MinidumpDirectoryEntries* directory_;

  
  MinidumpStreamMap*        stream_map_;

  
  
  const string              path_;

  
  
  std::istream*             stream_;

  
  
  
  
  bool                      swap_;

  
  
  
  bool                      valid_;
};


}  


#endif  
