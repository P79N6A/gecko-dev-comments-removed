


































#include <assert.h>
#include <fcntl.h>
#include <mach-o/arch.h>
#include <mach-o/loader.h>
#include <mach-o/swap.h>
#include <string.h>
#include <unistd.h>

#include "common/mac/macho_walker.h"
#include "common/mac/macho_utilities.h"

namespace MacFileUtilities {

MachoWalker::MachoWalker(const char *path, LoadCommandCallback callback,
                         void *context)
    : callback_(callback),
      callback_context_(context) {
  file_ = open(path, O_RDONLY);
}

MachoWalker::~MachoWalker() {
  if (file_ != -1)
    close(file_);
}

int MachoWalker::ValidateCPUType(int cpu_type) {
  
  
  if (cpu_type == 0) {
    const NXArchInfo *arch = NXGetLocalArchInfo();
    if (arch)
      cpu_type = arch->cputype;
    else
#if __ppc__
      cpu_type = CPU_TYPE_POWERPC;
#elif __i386__
    cpu_type = CPU_TYPE_X86;
#else
#error Unknown architecture -- are you on a PDP-11?
#endif
  }

  return cpu_type;
}

bool MachoWalker::WalkHeader(int cpu_type) {
  int valid_cpu_type = ValidateCPUType(cpu_type);
  off_t offset;
  if (FindHeader(valid_cpu_type, offset)) {
    if (cpu_type & CPU_ARCH_ABI64)
      return WalkHeader64AtOffset(offset);

    return WalkHeaderAtOffset(offset);
  }

  return false;
}

bool MachoWalker::ReadBytes(void *buffer, size_t size, off_t offset) {
  return pread(file_, buffer, size, offset) == (ssize_t)size;
}

bool MachoWalker::CurrentHeader(struct mach_header_64 *header, off_t *offset) {
  if (current_header_) {
    memcpy(header, current_header_, sizeof(mach_header_64));
    *offset = current_header_offset_;
    return true;
  }
  
  return false;
}

bool MachoWalker::FindHeader(int cpu_type, off_t &offset) {
  int valid_cpu_type = ValidateCPUType(cpu_type);
  
  uint32_t magic;
  if (!ReadBytes(&magic, sizeof(magic), 0))
    return false;

  offset = sizeof(magic);

  
  bool is_fat = false;
  if (magic == FAT_MAGIC || magic == FAT_CIGAM) {
    is_fat = true;    
  }
  else if (magic != MH_MAGIC && magic != MH_CIGAM && magic != MH_MAGIC_64 &&
           magic != MH_CIGAM_64) {
    return false;
  }

  if (!is_fat) {
    
    
    cpu_type_t header_cpu_type;
    if (!ReadBytes(&header_cpu_type, sizeof(header_cpu_type), offset))
      return false;

    if (magic == MH_CIGAM || magic == MH_CIGAM_64)
      header_cpu_type = NXSwapInt(header_cpu_type);

    if (valid_cpu_type != header_cpu_type)
      return false;

    offset = 0;
    return true;
  } else {
    
    offset = 0;
    struct fat_header fat;
    if (!ReadBytes(&fat, sizeof(fat), offset))
      return false;

    if (NXHostByteOrder() != NX_BigEndian)
      swap_fat_header(&fat, NXHostByteOrder());

    offset += sizeof(fat);

    
    struct fat_arch arch;
    for (uint32_t i = 0; i < fat.nfat_arch; ++i) {
      if (!ReadBytes(&arch, sizeof(arch), offset))
        return false;

      if (NXHostByteOrder() != NX_BigEndian)
        swap_fat_arch(&arch, 1, NXHostByteOrder());

      if (arch.cputype == valid_cpu_type) {
        offset = arch.offset;
        return true;
      }

      offset += sizeof(arch);
    }
  }

  return false;
}

bool MachoWalker::WalkHeaderAtOffset(off_t offset) {
  struct mach_header header;
  if (!ReadBytes(&header, sizeof(header), offset))
    return false;

  bool swap = (header.magic == MH_CIGAM);
  if (swap)
    swap_mach_header(&header, NXHostByteOrder());
  
  
  
  struct mach_header_64 header64;
  memcpy((void *)&header64, (const void *)&header, sizeof(header));
  header64.reserved = 0;
  
  current_header_ = &header64;
  current_header_size_ = sizeof(header); 
  current_header_offset_ = offset;
  offset += current_header_size_;
  bool result = WalkHeaderCore(offset, header.ncmds, swap);
  current_header_ = NULL;
  current_header_size_ = 0;
  current_header_offset_ = 0;
  return result;
}

bool MachoWalker::WalkHeader64AtOffset(off_t offset) {
  struct mach_header_64 header;
  if (!ReadBytes(&header, sizeof(header), offset))
    return false;

  bool swap = (header.magic == MH_CIGAM_64);
  if (swap)
    breakpad_swap_mach_header_64(&header, NXHostByteOrder());

  current_header_ = &header;
  current_header_size_ = sizeof(header);
  current_header_offset_ = offset;
  offset += current_header_size_;
  bool result = WalkHeaderCore(offset, header.ncmds, swap);
  current_header_ = NULL;
  current_header_size_ = 0;
  current_header_offset_ = 0;
  return result;
}

bool MachoWalker::WalkHeaderCore(off_t offset, uint32_t number_of_commands,
                                 bool swap) {
  for (uint32_t i = 0; i < number_of_commands; ++i) {
    struct load_command cmd;
    if (!ReadBytes(&cmd, sizeof(cmd), offset))
      return false;

    if (swap)
      swap_load_command(&cmd, NXHostByteOrder());

    
    if (callback_ && !callback_(this, &cmd, offset, swap, callback_context_))
      break;

    offset += cmd.cmdsize;
  }

  return true;
}

}  
