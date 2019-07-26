




























#ifndef CLIENT_WINDOWS_UNITTESTS_DUMP_ANALYSIS_H_
#define CLIENT_WINDOWS_UNITTESTS_DUMP_ANALYSIS_H_

#include "client/windows/crash_generation/minidump_generator.h"


struct FakeTEB {
  char dummy[0x30];
  void* peb;
};

class DumpAnalysis {
 public:
  explicit DumpAnalysis(const std::wstring& file_path)
      : dump_file_(file_path), dump_file_view_(NULL), dump_file_mapping_(NULL),
        dump_file_handle_(NULL) {
    EnsureDumpMapped();
  }
  ~DumpAnalysis();

  bool HasStream(ULONG stream_number) const;

  
  
  
  template <class StreamType>
  size_t GetStream(ULONG stream_number, StreamType** stream) const {
    return GetStreamImpl(stream_number, reinterpret_cast<void**>(stream));
  }

  bool HasTebs() const;
  bool HasPeb() const;
  bool HasMemory(ULONG64 address) const {
    return HasMemory<BYTE>(address, NULL);
  }

  bool HasMemory(const void* address) const {
    return HasMemory<BYTE>(address, NULL);
  }

  template <class StructureType>
  bool HasMemory(ULONG64 address, StructureType** structure = NULL) const {
    
    if (address > 0xFFFFFFFFUL)
      return false;

    return HasMemory(reinterpret_cast<void*>(address), structure);
  }

  template <class StructureType>
  bool HasMemory(const void* addr_in, StructureType** structure = NULL) const {
    return HasMemoryImpl(addr_in, sizeof(StructureType),
                             reinterpret_cast<void**>(structure));
  }

 protected:
  void EnsureDumpMapped();

  HANDLE dump_file_mapping_;
  HANDLE dump_file_handle_;
  void* dump_file_view_;
  std::wstring dump_file_;

 private:
  
  size_t GetStreamImpl(ULONG stream_number, void** stream) const;

  
  bool HasMemoryImpl(const void* addr_in, size_t pointersize,
                     void** structure) const;
};

#endif  
