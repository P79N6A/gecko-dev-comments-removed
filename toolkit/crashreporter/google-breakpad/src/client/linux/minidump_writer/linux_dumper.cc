




































#include "client/linux/minidump_writer/linux_dumper.h"

#include <assert.h>
#include <fcntl.h>
#include <limits.h>
#include <stddef.h>
#include <string.h>

#include "client/linux/minidump_writer/line_reader.h"
#include "common/linux/file_id.h"
#include "common/linux/linux_libc_support.h"
#include "common/linux/memory_mapped_file.h"
#include "common/linux/safe_readlink.h"
#include "third_party/lss/linux_syscall_support.h"

static const char kMappedFileUnsafePrefix[] = "/dev/";
static const char kDeletedSuffix[] = " (deleted)";

inline static bool IsMappedFileOpenUnsafe(
    const google_breakpad::MappingInfo& mapping) {
  
  
  
  
  return my_strncmp(mapping.name,
                    kMappedFileUnsafePrefix,
                    sizeof(kMappedFileUnsafePrefix) - 1) == 0;
}

namespace google_breakpad {


#define AT_MAX AT_SYSINFO_EHDR

LinuxDumper::LinuxDumper(pid_t pid)
    : pid_(pid),
      crash_address_(0),
      crash_signal_(0),
      crash_thread_(0),
      threads_(&allocator_, 8),
      mappings_(&allocator_),
      auxv_(&allocator_, AT_MAX + 1) {
}

LinuxDumper::~LinuxDumper() {
}

bool LinuxDumper::Init() {
  return ReadAuxv() && EnumerateThreads() && EnumerateMappings();
}

bool
LinuxDumper::ElfFileIdentifierForMapping(const MappingInfo& mapping,
                                         bool member,
                                         unsigned int mapping_id,
                                         uint8_t identifier[sizeof(MDGUID)])
{
  assert(!member || mapping_id < mappings_.size());
  my_memset(identifier, 0, sizeof(MDGUID));
  if (IsMappedFileOpenUnsafe(mapping))
    return false;

  
  if (my_strcmp(mapping.name, kLinuxGateLibraryName) == 0) {
    void* linux_gate = NULL;
    if (pid_ == sys_getpid()) {
      linux_gate = reinterpret_cast<void*>(mapping.start_addr);
    } else {
      linux_gate = allocator_.Alloc(mapping.size);
      CopyFromProcess(linux_gate, pid_,
                      reinterpret_cast<const void*>(mapping.start_addr),
                      mapping.size);
    }
    return FileID::ElfFileIdentifierFromMappedFile(linux_gate, identifier);
  }

  char filename[NAME_MAX];
  size_t filename_len = my_strlen(mapping.name);
  assert(filename_len < NAME_MAX);
  if (filename_len >= NAME_MAX)
    return false;
  my_memcpy(filename, mapping.name, filename_len);
  filename[filename_len] = '\0';
  bool filename_modified = HandleDeletedFileInMapping(filename);

  MemoryMappedFile mapped_file(filename);
  if (!mapped_file.data())  
    return false;

  bool success =
      FileID::ElfFileIdentifierFromMappedFile(mapped_file.data(), identifier);
  if (success && member && filename_modified) {
    mappings_[mapping_id]->name[filename_len -
                                sizeof(kDeletedSuffix) + 1] = '\0';
  }

  return success;
}

bool LinuxDumper::ReadAuxv() {
  char auxv_path[NAME_MAX];
  if (!BuildProcPath(auxv_path, pid_, "auxv")) {
    return false;
  }

  int fd = sys_open(auxv_path, O_RDONLY, 0);
  if (fd < 0) {
    return false;
  }

  elf_aux_entry one_aux_entry;
  bool res = false;
  while (sys_read(fd,
                  &one_aux_entry,
                  sizeof(elf_aux_entry)) == sizeof(elf_aux_entry) &&
         one_aux_entry.a_type != AT_NULL) {
    if (one_aux_entry.a_type <= AT_MAX) {
      auxv_[one_aux_entry.a_type] = one_aux_entry.a_un.a_val;
      res = true;
    }
  }
  sys_close(fd);
  return res;
}

bool LinuxDumper::EnumerateMappings() {
  char maps_path[NAME_MAX];
  if (!BuildProcPath(maps_path, pid_, "maps"))
    return false;

  
  
  
  
  
  
  
  const void* linux_gate_loc =
      reinterpret_cast<void *>(auxv_[AT_SYSINFO_EHDR]);
  
  
  
  const void* entry_point_loc = reinterpret_cast<void *>(auxv_[AT_ENTRY]);

  const int fd = sys_open(maps_path, O_RDONLY, 0);
  if (fd < 0)
    return false;
  LineReader* const line_reader = new(allocator_) LineReader(fd);

  const char* line;
  unsigned line_len;
  while (line_reader->GetNextLine(&line, &line_len)) {
    uintptr_t start_addr, end_addr, offset;

    const char* i1 = my_read_hex_ptr(&start_addr, line);
    if (*i1 == '-') {
      const char* i2 = my_read_hex_ptr(&end_addr, i1 + 1);
      if (*i2 == ' ') {
        const char* i3 = my_read_hex_ptr(&offset, i2 + 6 );
        if (*i3 == ' ') {
          const char* name = NULL;
          
          
          if (((name = my_strchr(line, '/')) == NULL) &&
              linux_gate_loc &&
              reinterpret_cast<void*>(start_addr) == linux_gate_loc) {
            name = kLinuxGateLibraryName;
            offset = 0;
          }
          
          
          if (name && !mappings_.empty()) {
            MappingInfo* module = mappings_.back();
            if ((start_addr == module->start_addr + module->size) &&
                (my_strlen(name) == my_strlen(module->name)) &&
                (my_strncmp(name, module->name, my_strlen(name)) == 0)) {
              module->size = end_addr - module->start_addr;
              line_reader->PopLine(line_len);
              continue;
            }
          }
          MappingInfo* const module = new(allocator_) MappingInfo;
          my_memset(module, 0, sizeof(MappingInfo));
          module->start_addr = start_addr;
          module->size = end_addr - start_addr;
          module->offset = offset;
          if (name != NULL) {
            const unsigned l = my_strlen(name);
            if (l < sizeof(module->name))
              my_memcpy(module->name, name, l);
          }
          
          
          
          
          
          if (entry_point_loc &&
              (entry_point_loc >=
                  reinterpret_cast<void*>(module->start_addr)) &&
              (entry_point_loc <
                  reinterpret_cast<void*>(module->start_addr+module->size)) &&
              !mappings_.empty()) {
            
            mappings_.resize(mappings_.size() + 1);
            for (size_t idx = mappings_.size() - 1; idx > 0; idx--)
              mappings_[idx] = mappings_[idx - 1];
            mappings_[0] = module;
          } else {
            mappings_.push_back(module);
          }
        }
      }
    }
    line_reader->PopLine(line_len);
  }

  sys_close(fd);

  return !mappings_.empty();
}




bool LinuxDumper::GetStackInfo(const void** stack, size_t* stack_len,
                               uintptr_t int_stack_pointer) {
  
  const uintptr_t page_size = getpagesize();

  uint8_t* const stack_pointer =
      reinterpret_cast<uint8_t*>(int_stack_pointer & ~(page_size - 1));

  
  static const ptrdiff_t kStackToCapture = 32 * 1024;

  const MappingInfo* mapping = FindMapping(stack_pointer);
  if (!mapping)
    return false;
  const ptrdiff_t offset = stack_pointer - (uint8_t*) mapping->start_addr;
  const ptrdiff_t distance_to_end =
      static_cast<ptrdiff_t>(mapping->size) - offset;
  *stack_len = distance_to_end > kStackToCapture ?
      kStackToCapture : distance_to_end;
  *stack = stack_pointer;
  return true;
}


const MappingInfo* LinuxDumper::FindMapping(const void* address) const {
  const uintptr_t addr = (uintptr_t) address;

  for (size_t i = 0; i < mappings_.size(); ++i) {
    const uintptr_t start = static_cast<uintptr_t>(mappings_[i]->start_addr);
    if (addr >= start && addr - start < mappings_[i]->size)
      return mappings_[i];
  }

  return NULL;
}

bool LinuxDumper::HandleDeletedFileInMapping(char* path) const {
  static const size_t kDeletedSuffixLen = sizeof(kDeletedSuffix) - 1;

  
  
  const size_t path_len = my_strlen(path);
  if (path_len < kDeletedSuffixLen + 2)
    return false;
  if (my_strncmp(path + path_len - kDeletedSuffixLen, kDeletedSuffix,
                 kDeletedSuffixLen) != 0) {
    return false;
  }

  
  char exe_link[NAME_MAX];
  char new_path[NAME_MAX];
  if (!BuildProcPath(exe_link, pid_, "exe"))
    return false;
  if (!SafeReadLink(exe_link, new_path))
    return false;
  if (my_strcmp(path, new_path) != 0)
    return false;

  
  struct kernel_stat exe_stat;
  struct kernel_stat new_path_stat;
  if (sys_stat(exe_link, &exe_stat) == 0 &&
      sys_stat(new_path, &new_path_stat) == 0 &&
      exe_stat.st_dev == new_path_stat.st_dev &&
      exe_stat.st_ino == new_path_stat.st_ino) {
    return false;
  }

  my_memcpy(path, exe_link, NAME_MAX);
  return true;
}

}  
