

























#include <elf.h>
#include <fcntl.h>
#include <unistd.h>

#include <sys/mman.h>
#include <sys/stat.h>

#if ELF_CLASS == ELFCLASS32
# define ELF_W(x)	ELF32_##x
# define Elf_W(x)	Elf32_##x
# define elf_w(x)	_Uelf32_##x
#else
# define ELF_W(x)	ELF64_##x
# define Elf_W(x)	Elf64_##x
# define elf_w(x)	_Uelf64_##x
#endif

#include "libunwind_i.h"
#include "../src/os-linux.h"

extern int elf_w (get_proc_name) (unw_addr_space_t as,
				  pid_t pid, unw_word_t ip,
				  char *buf, size_t len,
				  unw_word_t *offp);

static inline int
elf_w (valid_object) (struct elf_image *ei)
{
  if (ei->size <= EI_VERSION)
    return 0;

  return (memcmp (ei->image, ELFMAG, SELFMAG) == 0
	  && ((uint8_t *) ei->image)[EI_CLASS] == ELF_CLASS
	  && ((uint8_t *) ei->image)[EI_VERSION] != EV_NONE
	  && ((uint8_t *) ei->image)[EI_VERSION] <= EV_CURRENT);
}

static inline int
elf_map_image (struct elf_image *ei, const char *path)
{
  struct stat stat;
  int fd;

  fd = open_ashmem (path, O_RDONLY);
  if (fd < 0)
    return -1;

  if (fstat (fd, &stat) < 0)
    {
      close (fd);
      return -1;
    }

  ei->size = stat.st_size;
  ei->image = mmap (NULL, ei->size, PROT_READ, MAP_PRIVATE, fd, 0);
  close (fd);
  if (ei->image == MAP_FAILED)
    return -1;

  if (!elf_w (valid_object) (ei))
  {
    munmap(ei->image, ei->size);
    return -1;
  }

  return 0;
}
