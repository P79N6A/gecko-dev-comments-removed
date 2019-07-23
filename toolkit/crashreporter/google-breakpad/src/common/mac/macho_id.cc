


































#include <fcntl.h>
#include <mach-o/loader.h>
#include <mach-o/swap.h>
#include <openssl/md5.h>
#include <openssl/sha.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#include "common/mac/macho_id.h"
#include "common/mac/macho_walker.h"

namespace MacFileUtilities {

MachoID::MachoID(const char *path) {
  strlcpy(path_, path, sizeof(path_));
  file_ = open(path, O_RDONLY);
}

MachoID::~MachoID() {
  if (file_ != -1)
    close(file_);
}





#define MOD_ADLER 65521

#define MAX_BLOCK 5552

void MachoID::UpdateCRC(unsigned char *bytes, size_t size) {

#define DO1(buf,i)  {sum1 += (buf)[i]; sum2 += sum1;}
#define DO2(buf,i)  DO1(buf,i); DO1(buf,i+1);
#define DO4(buf,i)  DO2(buf,i); DO2(buf,i+2);
#define DO8(buf,i)  DO4(buf,i); DO4(buf,i+4);
#define DO16(buf)   DO8(buf,0); DO8(buf,8);
  
  uint32_t sum1 = crc_ & 0xFFFF;
  uint32_t sum2 = (crc_ >> 16) & 0xFFFF;

  
  while (size >= MAX_BLOCK) {
    size -= MAX_BLOCK;
    int block_count = MAX_BLOCK / 16;
    do {
      DO16(bytes);
      bytes += 16;
    } while (--block_count);
    sum1 %= MOD_ADLER;
    sum2 %= MOD_ADLER;
  }

  
  if (size) {
    while (size >= 16) {
      size -= 16;
      DO16(bytes);
      bytes += 16;
    }
    while (size--) {
      sum1 += *bytes++;
      sum2 += sum1;
    }
    sum1 %= MOD_ADLER;
    sum2 %= MOD_ADLER;
    crc_ = (sum2 << 16) | sum1;
  }
}

void MachoID::UpdateMD5(unsigned char *bytes, size_t size) {
  MD5_Update(&md5_context_, bytes, size);
}

void MachoID::UpdateSHA1(unsigned char *bytes, size_t size) {
  SHA_Update(&sha1_context_, bytes, size);
}

void MachoID::Update(MachoWalker *walker, unsigned long offset, size_t size) {
  if (!update_function_ || !size)
    return;

  
  unsigned char buffer[4096];
  size_t buffer_size;
  off_t file_offset = offset;
  while (size > 0) {
    if (size > sizeof(buffer)) {
      buffer_size = sizeof(buffer);
      size -= buffer_size;
    } else {
      buffer_size = size;
      size = 0;
    }

    if (!walker->ReadBytes(buffer, buffer_size, file_offset))
      return;

    (this->*update_function_)(buffer, buffer_size);
    file_offset += buffer_size;
  }
}

bool MachoID::UUIDCommand(int cpu_type, unsigned char bytes[16]) {
  struct uuid_command uuid_cmd;
  MachoWalker walker(path_, UUIDWalkerCB, &uuid_cmd);

  uuid_cmd.cmd = 0;
  if (!walker.WalkHeader(cpu_type))
    return false;

  
  
  if (uuid_cmd.cmd == LC_UUID) {
    memcpy(bytes, uuid_cmd.uuid, sizeof(uuid_cmd.uuid));
    return true;
  }

  return false;
}

bool MachoID::IDCommand(int cpu_type, unsigned char identifier[16]) {
  struct dylib_command dylib_cmd;
  MachoWalker walker(path_, IDWalkerCB, &dylib_cmd);

  dylib_cmd.cmd = 0;
  if (!walker.WalkHeader(cpu_type))
    return false;

  
  
  if (dylib_cmd.cmd == LC_ID_DYLIB) {
    
    

    
    identifier[0] = 0;
    identifier[1] = 0;
    identifier[2] = 0;
    identifier[3] = 0;

    for (int j = 0, i = strlen(path_)-1; i >= 0 && path_[i]!='/'; ++j, --i) {
      identifier[j%4] += path_[i];
    }

    identifier[4] = (dylib_cmd.dylib.current_version >> 24) & 0xFF;
    identifier[5] = (dylib_cmd.dylib.current_version >> 16) & 0xFF;
    identifier[6] = (dylib_cmd.dylib.current_version >> 8) & 0xFF;
    identifier[7] = dylib_cmd.dylib.current_version & 0xFF;
    identifier[8] = (dylib_cmd.dylib.compatibility_version >> 24) & 0xFF;
    identifier[9] = (dylib_cmd.dylib.compatibility_version >> 16) & 0xFF;
    identifier[10] = (dylib_cmd.dylib.compatibility_version >> 8) & 0xFF;
    identifier[11] = dylib_cmd.dylib.compatibility_version & 0xFF;
    identifier[12] = (cpu_type >> 24) & 0xFF;
    identifier[13] = (cpu_type >> 16) & 0xFF;
    identifier[14] = (cpu_type >> 8) & 0xFF;
    identifier[15] = cpu_type & 0xFF;

    return true;
  }

  return false;
}

uint32_t MachoID::Adler32(int cpu_type) {
  MachoWalker walker(path_, WalkerCB, this);
  update_function_ = &MachoID::UpdateCRC;
  crc_ = 0;

  if (!walker.WalkHeader(cpu_type))
    return 0;

  return crc_;
}

bool MachoID::MD5(int cpu_type, unsigned char identifier[16]) {
  MachoWalker walker(path_, WalkerCB, this);
  update_function_ = &MachoID::UpdateMD5;

  if (MD5_Init(&md5_context_)) {
    if (!walker.WalkHeader(cpu_type))
      return false;

    MD5_Final(identifier, &md5_context_);
    return true;
  }

  return false;
}

bool MachoID::SHA1(int cpu_type, unsigned char identifier[16]) {
  MachoWalker walker(path_, WalkerCB, this);
  update_function_ = &MachoID::UpdateSHA1;

  if (SHA_Init(&sha1_context_)) {
    if (!walker.WalkHeader(cpu_type))
      return false;

    SHA_Final(identifier, &sha1_context_);
    return true;
  }

  return false;
}


bool MachoID::WalkerCB(MachoWalker *walker, load_command *cmd, off_t offset,
                       bool swap, void *context) {
  MachoID *macho_id = (MachoID *)context;

  if (cmd->cmd == LC_SEGMENT) {
    struct segment_command seg;

    if (!walker->ReadBytes(&seg, sizeof(seg), offset))
      return false;

    if (swap)
      swap_segment_command(&seg, NXHostByteOrder());

    struct mach_header_64 header;
    off_t header_offset;
    
    if (!walker->CurrentHeader(&header, &header_offset))
      return false;
        
    
    
    offset += sizeof(struct segment_command);
    struct section sec;
    for (unsigned long i = 0; i < seg.nsects; ++i) {
      if (!walker->ReadBytes(&sec, sizeof(sec), offset))
        return false;

      if (swap)
        swap_section(&sec, 1, NXHostByteOrder());

      macho_id->Update(walker, header_offset + sec.offset, sec.size);
      offset += sizeof(struct section);
    }
  } else if (cmd->cmd == LC_SEGMENT_64) {
    struct segment_command_64 seg64;

    if (!walker->ReadBytes(&seg64, sizeof(seg64), offset))
      return false;

    if (swap)
      swap_segment_command_64(&seg64, NXHostByteOrder());

    struct mach_header_64 header;
    off_t header_offset;
    
    if (!walker->CurrentHeader(&header, &header_offset))
      return false;
    
    
    
    offset += sizeof(struct segment_command_64);
    struct section_64 sec64;
    for (unsigned long i = 0; i < seg64.nsects; ++i) {
      if (!walker->ReadBytes(&sec64, sizeof(sec64), offset))
        return false;

      if (swap)
        swap_section_64(&sec64, 1, NXHostByteOrder());

      macho_id->Update(walker, header_offset + sec64.offset, sec64.size);
      offset += sizeof(struct section_64);
    }
  }

  
  return true;
}


bool MachoID::UUIDWalkerCB(MachoWalker *walker, load_command *cmd, off_t offset,
                           bool swap, void *context) {
  if (cmd->cmd == LC_UUID) {
    struct uuid_command *uuid_cmd = (struct uuid_command *)context;

    if (!walker->ReadBytes(uuid_cmd, sizeof(struct uuid_command), offset))
      return false;

    if (swap)
      swap_uuid_command(uuid_cmd, NXHostByteOrder());

    return false;
  }

  
  return true;
}


bool MachoID::IDWalkerCB(MachoWalker *walker, load_command *cmd, off_t offset,
                         bool swap, void *context) {
  if (cmd->cmd == LC_ID_DYLIB) {
    struct dylib_command *dylib_cmd = (struct dylib_command *)context;

    if (!walker->ReadBytes(dylib_cmd, sizeof(struct dylib_command), offset))
      return false;

    if (swap)
      swap_dylib_command(dylib_cmd, NXHostByteOrder());

    return false;
  }

  
  return true;
}

}  
