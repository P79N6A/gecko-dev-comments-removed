#include <stdio.h>

#include "google_breakpad/processor/minidump.h"
#include "nscore.h"

using namespace google_breakpad;


extern "C"
NS_EXPORT bool
DumpHasStream(const char* dump_file, uint32_t stream_type)
{
  Minidump dump(dump_file);
  if (!dump.Read())
    return false;

  uint32_t length;
  if (!dump.SeekToStreamType(stream_type, &length) || length == 0)
    return false;

  return true;
}



extern "C"
NS_EXPORT bool
DumpHasInstructionPointerMemory(const char* dump_file)
{
  Minidump minidump(dump_file);
  if (!minidump.Read())
    return false;

  MinidumpException* exception = minidump.GetException();
  MinidumpMemoryList* memory_list = minidump.GetMemoryList();
  if (!exception || !memory_list) {
    return false;
  }

  MinidumpContext* context = exception->GetContext();
  if (!context)
    return false;

  uint64_t instruction_pointer;
  if (!context->GetInstructionPointer(&instruction_pointer)) {
    return false;
  }

  MinidumpMemoryRegion* region =
    memory_list->GetMemoryRegionForAddress(instruction_pointer);
  return region != NULL;
}






extern "C"
NS_EXPORT bool
DumpCheckMemory(const char* dump_file)
{
  Minidump dump(dump_file);
  if (!dump.Read())
    return false;

  MinidumpMemoryList* memory_list = dump.GetMemoryList();
  if (!memory_list) {
    return false;
  }

  void *addr;
  FILE *fp = fopen("crash-addr", "r");
  if (!fp)
    return false;
  if (fscanf(fp, "%p", &addr) != 1)
    return false;
  fclose(fp);

  remove("crash-addr");

  MinidumpMemoryRegion* region =
    memory_list->GetMemoryRegionForAddress(uint64_t(addr));
  if(!region)
    return false;

  const uint8_t* chars = region->GetMemory();
  if (region->GetSize() != 32)
    return false;

  for (int i=0; i<32; i++) {
    if (chars[i] != i)
      return false;
  }

  return true;
}
