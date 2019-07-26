




























#include <windows.h>
#include <objbase.h>
#include <dbghelp.h>

#include "client/windows/unittests/dump_analysis.h"  
#include "testing/gtest/include/gtest/gtest.h"

DumpAnalysis::~DumpAnalysis() {
  if (dump_file_view_ != NULL) {
    EXPECT_TRUE(::UnmapViewOfFile(dump_file_view_));
    ::CloseHandle(dump_file_mapping_);
    dump_file_mapping_ = NULL;
  }

  if (dump_file_handle_ != NULL) {
    ::CloseHandle(dump_file_handle_);
    dump_file_handle_ = NULL;
  }
}

void DumpAnalysis::EnsureDumpMapped() {
  if (dump_file_view_ == NULL) {
    dump_file_handle_ = ::CreateFile(dump_file_.c_str(),
      GENERIC_READ,
      0,
      NULL,
      OPEN_EXISTING,
      0,
      NULL);
    ASSERT_TRUE(dump_file_handle_ != NULL);
    ASSERT_TRUE(dump_file_mapping_ == NULL);

    dump_file_mapping_ = ::CreateFileMapping(dump_file_handle_,
      NULL,
      PAGE_READONLY,
      0,
      0,
      NULL);
    ASSERT_TRUE(dump_file_mapping_ != NULL);

    dump_file_view_ = ::MapViewOfFile(dump_file_mapping_,
      FILE_MAP_READ,
      0,
      0,
      0);
    ASSERT_TRUE(dump_file_view_ != NULL);
  }
}

bool DumpAnalysis::HasTebs() const {
  MINIDUMP_THREAD_LIST* thread_list = NULL;
  size_t thread_list_size = GetStream(ThreadListStream, &thread_list);

  if (thread_list_size > 0 && thread_list != NULL) {
    for (ULONG i = 0; i < thread_list->NumberOfThreads; ++i) {
      if (!HasMemory(thread_list->Threads[i].Teb))
        return false;
    }

    return true;
  }

  
  return false;
}

bool DumpAnalysis::HasPeb() const {
  MINIDUMP_THREAD_LIST* thread_list = NULL;
  size_t thread_list_size = GetStream(ThreadListStream, &thread_list);

  if (thread_list_size > 0 && thread_list != NULL &&
      thread_list->NumberOfThreads > 0) {
    FakeTEB* teb = NULL;
    if (!HasMemory(thread_list->Threads[0].Teb, &teb))
      return false;

    return HasMemory(teb->peb);
  }

  return false;
}

bool DumpAnalysis::HasStream(ULONG stream_number) const {
  void* stream = NULL;
  size_t stream_size = GetStreamImpl(stream_number, &stream);
  return stream_size > 0 && stream != NULL;
}

size_t DumpAnalysis::GetStreamImpl(ULONG stream_number, void** stream) const {
  MINIDUMP_DIRECTORY* directory = NULL;
  ULONG memory_list_size = 0;
  BOOL ret = ::MiniDumpReadDumpStream(dump_file_view_,
                                      stream_number,
                                      &directory,
                                      stream,
                                      &memory_list_size);

  return ret ? memory_list_size : 0;
}

bool DumpAnalysis::HasMemoryImpl(const void *addr_in, size_t structuresize,
                                 void **structure) const {
  uintptr_t address = reinterpret_cast<uintptr_t>(addr_in);
  MINIDUMP_MEMORY_LIST* memory_list = NULL;
  size_t memory_list_size = GetStream(MemoryListStream, &memory_list);
  if (memory_list_size > 0 && memory_list != NULL) {
    for (ULONG i = 0; i < memory_list->NumberOfMemoryRanges; ++i) {
      MINIDUMP_MEMORY_DESCRIPTOR& descr = memory_list->MemoryRanges[i];
      const uintptr_t range_start =
          static_cast<uintptr_t>(descr.StartOfMemoryRange);
      uintptr_t range_end = range_start + descr.Memory.DataSize;

      if (address >= range_start &&
          address + structuresize < range_end) {
        
        
        if (structure != NULL)
          *structure = RVA_TO_ADDR(dump_file_view_, descr.Memory.Rva);

        return true;
      }
    }
  }

  
  
  
  MINIDUMP_MEMORY64_LIST* memory64_list = NULL;
  memory_list_size = GetStream(Memory64ListStream, &memory64_list);
  if (memory_list_size > 0 && memory64_list != NULL) {
    
    RVA64 curr_rva = memory64_list->BaseRva;
    for (ULONG i = 0; i < memory64_list->NumberOfMemoryRanges; ++i) {
      MINIDUMP_MEMORY_DESCRIPTOR64& descr = memory64_list->MemoryRanges[i];
      uintptr_t range_start =
          static_cast<uintptr_t>(descr.StartOfMemoryRange);
      uintptr_t range_end = range_start + static_cast<size_t>(descr.DataSize);

      if (address >= range_start &&
          address + structuresize < range_end) {
        
        
        if (structure != NULL)
          *structure = RVA_TO_ADDR(dump_file_view_, curr_rva);

        return true;
      }

      
      curr_rva += descr.DataSize;
    }
  }

  return false;
}
