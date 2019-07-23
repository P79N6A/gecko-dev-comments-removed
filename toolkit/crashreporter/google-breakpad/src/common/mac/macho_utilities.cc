
































#include "common/mac/macho_utilities.h"

void breakpad_swap_uuid_command(struct breakpad_uuid_command *uc,
                                enum NXByteOrder target_byte_order)
{
  uc->cmd = NXSwapLong(uc->cmd);
  uc->cmdsize = NXSwapLong(uc->cmdsize);
}

void breakpad_swap_segment_command_64(struct segment_command_64 *sg,
                                      enum NXByteOrder target_byte_order)
{
  sg->cmd = NXSwapLong(sg->cmd);
  sg->cmdsize = NXSwapLong(sg->cmdsize);

  sg->vmaddr = NXSwapLongLong(sg->vmaddr);
  sg->vmsize = NXSwapLongLong(sg->vmsize);
  sg->fileoff = NXSwapLongLong(sg->fileoff);
  sg->filesize = NXSwapLongLong(sg->filesize);

  sg->maxprot = NXSwapLong(sg->maxprot);
  sg->initprot = NXSwapLong(sg->initprot);
  sg->nsects = NXSwapLong(sg->nsects);
  sg->flags = NXSwapLong(sg->flags);
}

void breakpad_swap_mach_header_64(struct mach_header_64 *mh,
                                  enum NXByteOrder target_byte_order)
{
  mh->magic = NXSwapLong(mh->magic);
  mh->cputype = NXSwapLong(mh->cputype);
  mh->cpusubtype = NXSwapLong(mh->cpusubtype);
  mh->filetype = NXSwapLong(mh->filetype);
  mh->ncmds = NXSwapLong(mh->ncmds);
  mh->sizeofcmds = NXSwapLong(mh->sizeofcmds);
  mh->flags = NXSwapLong(mh->flags);
  mh->reserved = NXSwapLong(mh->reserved);
}

void breakpad_swap_section_64(struct section_64 *s,
                              uint32_t nsects,
                              enum NXByteOrder target_byte_order)
{
  for (uint32_t i = 0; i < nsects; i++) {
    s[i].addr = NXSwapLongLong(s[i].addr);
    s[i].size = NXSwapLongLong(s[i].size);

    s[i].offset = NXSwapLong(s[i].offset);
    s[i].align = NXSwapLong(s[i].align);
    s[i].reloff = NXSwapLong(s[i].reloff);
    s[i].nreloc = NXSwapLong(s[i].nreloc);
    s[i].flags = NXSwapLong(s[i].flags);
    s[i].reserved1 = NXSwapLong(s[i].reserved1);
    s[i].reserved2 = NXSwapLong(s[i].reserved2);
  }
}
