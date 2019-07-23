













































































#ifndef GOOGLE_BREAKPAD_PROCESSOR_MINIDUMP_H__
#define GOOGLE_BREAKPAD_PROCESSOR_MINIDUMP_H__

#include <unistd.h>

#include <map>
#include <string>
#include <vector>

#include "google_breakpad/common/minidump_format.h"
#include "google_breakpad/processor/code_module.h"
#include "google_breakpad/processor/code_modules.h"
#include "google_breakpad/processor/memory_region.h"


namespace google_breakpad {


using std::map;
using std::string;
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
  
  
  
  
  
  
  
  virtual bool Read(u_int32_t expected_size) = 0;
};












class MinidumpContext : public MinidumpStream {
 public:
  virtual ~MinidumpContext();

  
  
  
  
  u_int32_t GetContextCPU() const;

  
  
  
  const MDRawContextX86*   GetContextX86() const;
  const MDRawContextPPC*   GetContextPPC() const;
  const MDRawContextAMD64* GetContextAMD64() const;
  const MDRawContextSPARC* GetContextSPARC() const;
 
  
  void Print();

 private:
  friend class MinidumpThread;
  friend class MinidumpException;

  explicit MinidumpContext(Minidump* minidump);

  bool Read(u_int32_t expected_size);

  
  void FreeContext();

  
  
  
  
  
  bool CheckAgainstSystemInfo(u_int32_t context_cpu_type);

  
  u_int32_t context_flags_;

  
  union {
    MDRawContextBase*  base;
    MDRawContextX86*   x86;
    MDRawContextPPC*   ppc;
    MDRawContextAMD64* amd64;
    
    
    MDRawContextSPARC*  ctx_sparc;
  } context_;
};










class MinidumpMemoryRegion : public MinidumpObject,
                             public MemoryRegion {
 public:
  virtual ~MinidumpMemoryRegion();

  static void set_max_bytes(u_int32_t max_bytes) { max_bytes_ = max_bytes; }
  static u_int32_t max_bytes() { return max_bytes_; }

  
  
  
  const u_int8_t* GetMemory();

  
  u_int64_t GetBase();

  
  u_int32_t GetSize();

  
  void FreeMemory();

  
  bool GetMemoryAtAddress(u_int64_t address, u_int8_t*  value);
  bool GetMemoryAtAddress(u_int64_t address, u_int16_t* value);
  bool GetMemoryAtAddress(u_int64_t address, u_int32_t* value);
  bool GetMemoryAtAddress(u_int64_t address, u_int64_t* value);

  
  void Print();

 private:
  friend class MinidumpThread;
  friend class MinidumpMemoryList;

  explicit MinidumpMemoryRegion(Minidump* minidump);

  
  
  void SetDescriptor(MDMemoryDescriptor* descriptor);

  
  template<typename T> bool GetMemoryAtAddressInternal(u_int64_t address,
                                                       T*        value);

  
  
  static u_int32_t max_bytes_;

  
  
  MDMemoryDescriptor* descriptor_;

  
  vector<u_int8_t>*   memory_;
};







class MinidumpThread : public MinidumpObject {
 public:
  virtual ~MinidumpThread();

  const MDRawThread* thread() const { return valid_ ? &thread_ : NULL; }
  MinidumpMemoryRegion* GetMemory();
  MinidumpContext* GetContext();

  
  
  
  
  bool GetThreadID(u_int32_t *thread_id) const;

  
  void Print();

 private:
  
  friend class MinidumpThreadList;

  explicit MinidumpThread(Minidump* minidump);

  
  
  
  bool Read();

  MDRawThread           thread_;
  MinidumpMemoryRegion* memory_;
  MinidumpContext*      context_;
};




class MinidumpThreadList : public MinidumpStream {
 public:
  virtual ~MinidumpThreadList();

  static void set_max_threads(u_int32_t max_threads) {
    max_threads_ = max_threads;
  }
  static u_int32_t max_threads() { return max_threads_; }

  unsigned int thread_count() const {
    return valid_ ? thread_count_ : 0;
  }

  
  MinidumpThread* GetThreadAtIndex(unsigned int index) const;

  
  MinidumpThread* GetThreadByID(u_int32_t thread_id);

  
  void Print();

 private:
  friend class Minidump;

  typedef map<u_int32_t, MinidumpThread*> IDToThreadMap;
  typedef vector<MinidumpThread> MinidumpThreads;

  static const u_int32_t kStreamType = MD_THREAD_LIST_STREAM;

  explicit MinidumpThreadList(Minidump* aMinidump);

  bool Read(u_int32_t aExpectedSize);

  
  
  static u_int32_t max_threads_;

  
  IDToThreadMap    id_to_thread_map_;

  
  MinidumpThreads* threads_;
  u_int32_t        thread_count_;
};






class MinidumpModule : public MinidumpObject,
                       public CodeModule {
 public:
  virtual ~MinidumpModule();

  static void set_max_cv_bytes(u_int32_t max_cv_bytes) {
    max_cv_bytes_ = max_cv_bytes;
  }
  static u_int32_t max_cv_bytes() { return max_cv_bytes_; }

  static void set_max_misc_bytes(u_int32_t max_misc_bytes) {
    max_misc_bytes_ = max_misc_bytes;
  }
  static u_int32_t max_misc_bytes() { return max_misc_bytes_; }

  const MDRawModule* module() const { return valid_ ? &module_ : NULL; }

  
  virtual u_int64_t base_address() const {
    return valid_ ? module_.base_of_image : static_cast<u_int64_t>(-1);
  }
  virtual u_int64_t size() const { return valid_ ? module_.size_of_image : 0; }
  virtual string code_file() const;
  virtual string code_identifier() const;
  virtual string debug_file() const;
  virtual string debug_identifier() const;
  virtual string version() const;
  virtual const CodeModule* Copy() const;

  
  
  
  
  
  
  
  
  
  
  const u_int8_t* GetCVRecord(u_int32_t* size);

  
  
  
  
  
  const MDImageDebugMisc* GetMiscRecord(u_int32_t* size);

  
  void Print();

 private:
  
  friend class MinidumpModuleList;

  explicit MinidumpModule(Minidump* minidump);

  
  
  
  bool Read();

  
  
  
  
  
  
  bool ReadAuxiliaryData();

  
  
  
  static u_int32_t max_cv_bytes_;
  static u_int32_t max_misc_bytes_;

  
  
  
  
  
  bool              module_valid_;

  
  
  
  bool              has_debug_info_;

  MDRawModule       module_;

  
  const string*     name_;

  
  
  
  
  vector<u_int8_t>* cv_record_;

  
  
  
  u_int32_t cv_record_signature_;

  
  
  
  vector<u_int8_t>* misc_record_;
};






class MinidumpModuleList : public MinidumpStream,
                           public CodeModules {
 public:
  virtual ~MinidumpModuleList();

  static void set_max_modules(u_int32_t max_modules) {
    max_modules_ = max_modules;
  }
  static u_int32_t max_modules() { return max_modules_; }

  
  virtual unsigned int module_count() const {
    return valid_ ? module_count_ : 0;
  }
  virtual const MinidumpModule* GetModuleForAddress(u_int64_t address) const;
  virtual const MinidumpModule* GetMainModule() const;
  virtual const MinidumpModule* GetModuleAtSequence(
      unsigned int sequence) const;
  virtual const MinidumpModule* GetModuleAtIndex(unsigned int index) const;
  virtual const CodeModules* Copy() const;

  
  void Print();

 private:
  friend class Minidump;

  typedef vector<MinidumpModule> MinidumpModules;

  static const u_int32_t kStreamType = MD_MODULE_LIST_STREAM;

  explicit MinidumpModuleList(Minidump* minidump);

  bool Read(u_int32_t expected_size);

  
  
  static u_int32_t max_modules_;

  
  RangeMap<u_int64_t, unsigned int> *range_map_;

  MinidumpModules *modules_;
  u_int32_t module_count_;
};











class MinidumpMemoryList : public MinidumpStream {
 public:
  virtual ~MinidumpMemoryList();

  static void set_max_regions(u_int32_t max_regions) {
    max_regions_ = max_regions;
  }
  static u_int32_t max_regions() { return max_regions_; }

  unsigned int region_count() const { return valid_ ? region_count_ : 0; }

  
  MinidumpMemoryRegion* GetMemoryRegionAtIndex(unsigned int index);

  
  
  MinidumpMemoryRegion* GetMemoryRegionForAddress(u_int64_t address);

  
  void Print();

 private:
  friend class Minidump;

  typedef vector<MDMemoryDescriptor>   MemoryDescriptors;
  typedef vector<MinidumpMemoryRegion> MemoryRegions;

  static const u_int32_t kStreamType = MD_MEMORY_LIST_STREAM;

  explicit MinidumpMemoryList(Minidump* minidump);

  bool Read(u_int32_t expected_size);

  
  
  static u_int32_t max_regions_;

  
  RangeMap<u_int64_t, unsigned int> *range_map_;

  
  
  
  
  MemoryDescriptors *descriptors_;

  
  MemoryRegions *regions_;
  u_int32_t region_count_;
};








class MinidumpException : public MinidumpStream {
 public:
  virtual ~MinidumpException();

  const MDRawExceptionStream* exception() const {
    return valid_ ? &exception_ : NULL;
  }

  
  
  
  
  bool GetThreadID(u_int32_t *thread_id) const;

  MinidumpContext* GetContext();

  
  void Print();

 private:
  friend class Minidump;

  static const u_int32_t kStreamType = MD_EXCEPTION_STREAM;

  explicit MinidumpException(Minidump* minidump);

  bool Read(u_int32_t expected_size);

  MDRawExceptionStream exception_;
  MinidumpContext*     context_;
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

 private:
  friend class Minidump;

  static const u_int32_t kStreamType = MD_SYSTEM_INFO_STREAM;

  explicit MinidumpSystemInfo(Minidump* minidump);

  bool Read(u_int32_t expected_size);

  MDRawSystemInfo system_info_;

  
  
  const string* csd_version_;

  
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

  static const u_int32_t kStreamType = MD_MISC_INFO_STREAM;

  explicit MinidumpMiscInfo(Minidump* minidump_);

  bool Read(u_int32_t expected_size_);

  MDRawMiscInfo misc_info_;
};





class MinidumpBreakpadInfo : public MinidumpStream {
 public:
  const MDRawBreakpadInfo* breakpad_info() const {
    return valid_ ? &breakpad_info_ : NULL;
  }

  
  
  
  
  bool GetDumpThreadID(u_int32_t *thread_id) const;
  bool GetRequestingThreadID(u_int32_t *thread_id) const;

  
  void Print();

 private:
  friend class Minidump;

  static const u_int32_t kStreamType = MD_BREAKPAD_INFO_STREAM;

  explicit MinidumpBreakpadInfo(Minidump* minidump_);

  bool Read(u_int32_t expected_size_);

  MDRawBreakpadInfo breakpad_info_;
};




class Minidump {
 public:
  
  explicit Minidump(const string& path);

  virtual ~Minidump();

  virtual string path() const {
    return path_;
  }
  static void set_max_streams(u_int32_t max_streams) {
    max_streams_ = max_streams;
  }
  static u_int32_t max_streams() { return max_streams_; }

  static void set_max_string_length(u_int32_t max_string_length) {
    max_string_length_ = max_string_length;
  }
  static u_int32_t max_string_length() { return max_string_length_; }

  virtual const MDRawHeader* header() const { return valid_ ? &header_ : NULL; }

  
  
  
  
  virtual bool Read();

  
  
  
  
  virtual MinidumpThreadList* GetThreadList();
  MinidumpModuleList* GetModuleList();
  MinidumpMemoryList* GetMemoryList();
  MinidumpException* GetException();
  MinidumpSystemInfo* GetSystemInfo();
  MinidumpMiscInfo* GetMiscInfo();
  MinidumpBreakpadInfo* GetBreakpadInfo();

  
  
  
  

  unsigned int GetDirectoryEntryCount() const {
    return valid_ ? header_.stream_count : 0;
  }
  const MDRawDirectory* GetDirectoryEntryAtIndex(unsigned int index) const;

  

  
  
  
  bool ReadBytes(void* bytes, size_t count);

  
  bool SeekSet(off_t offset);

  
  off_t Tell() { return valid_ ? lseek(fd_, 0, SEEK_CUR) : (off_t)-1; }

  

  
  
  
  string* ReadString(off_t offset);

  
  
  
  
  
  
  
  
  
  
  
  
  bool SeekToStreamType(u_int32_t stream_type, u_int32_t* stream_length);

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
  typedef map<u_int32_t, MinidumpStreamInfo> MinidumpStreamMap;

  template<typename T> T* GetStream(T** stream);

  
  bool Open();

  
  
  
  static u_int32_t max_streams_;

  
  
  
  
  static unsigned int max_string_length_;

  MDRawHeader               header_;

  
  MinidumpDirectoryEntries* directory_;

  
  MinidumpStreamMap*        stream_map_;

  
  const string              path_;

  
  
  int                       fd_;

  
  
  
  
  bool                      swap_;

  
  
  
  bool                      valid_;
};


}  


#endif  
