

































#include "common/mac/macho_reader.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>


#if !defined(CPU_TYPE_ARM)
#define CPU_TYPE_ARM 12
#endif

namespace google_breakpad {
namespace mach_o {









#if defined(NDEBUG)
#define ASSERT_ALWAYS_EVAL(x) (x)
#else
#define ASSERT_ALWAYS_EVAL(x) assert(x)
#endif

void FatReader::Reporter::BadHeader() {
  fprintf(stderr, "%s: file is neither a fat binary file"
          " nor a Mach-O object file\n", filename_.c_str());
}

void FatReader::Reporter::TooShort() {
  fprintf(stderr, "%s: file too short for the data it claims to contain\n",
          filename_.c_str());
}

void FatReader::Reporter::MisplacedObjectFile() {
  fprintf(stderr, "%s: file too short for the object files it claims"
          " to contain\n", filename_.c_str());
}

bool FatReader::Read(const uint8_t *buffer, size_t size) {
  buffer_.start = buffer;
  buffer_.end = buffer + size;
  ByteCursor cursor(&buffer_);

  
  
  
  
  cursor.set_big_endian(true);
  if (cursor >> magic_) {
    if (magic_ == FAT_MAGIC) {
      
      uint32_t object_files_count;
      if (!(cursor >> object_files_count)) {  
        reporter_->TooShort();
        return false;
      }

      
      object_files_.resize(object_files_count);
      for (size_t i = 0; i < object_files_count; i++) {
        struct fat_arch *objfile = &object_files_[i];

        
        cursor >> objfile->cputype
               >> objfile->cpusubtype
               >> objfile->offset
               >> objfile->size
               >> objfile->align;
        if (!cursor) {
          reporter_->TooShort();
          return false;
        }
        
        size_t fat_size = buffer_.Size();
        if (objfile->offset > fat_size ||
            objfile->size > fat_size - objfile->offset) {
          reporter_->MisplacedObjectFile();
          return false;
        }
      }

      return true;
    } else if (magic_ == MH_MAGIC || magic_ == MH_MAGIC_64 ||
               magic_ == MH_CIGAM || magic_ == MH_CIGAM_64) {
      
      if (magic_ == MH_CIGAM || magic_ == MH_CIGAM_64)
        cursor.set_big_endian(false);
      
      object_files_.resize(1);

      
      if (!(cursor >> object_files_[0].cputype
                   >> object_files_[0].cpusubtype)) {
        reporter_->TooShort();
        return false;
      }

      object_files_[0].offset = 0;
      object_files_[0].size = static_cast<uint32_t>(buffer_.Size());
      
      
      
      object_files_[0].align = 12;  
      
      return true;
    }
  }
  
  reporter_->BadHeader();
  return false;
}

void Reader::Reporter::BadHeader() {
  fprintf(stderr, "%s: file is not a Mach-O object file\n", filename_.c_str());
}

void Reader::Reporter::CPUTypeMismatch(cpu_type_t cpu_type,
                                       cpu_subtype_t cpu_subtype,
                                       cpu_type_t expected_cpu_type,
                                       cpu_subtype_t expected_cpu_subtype) {
  fprintf(stderr, "%s: CPU type %d, subtype %d does not match expected"
          " type %d, subtype %d\n",
          filename_.c_str(), cpu_type, cpu_subtype,
          expected_cpu_type, expected_cpu_subtype);
}

void Reader::Reporter::HeaderTruncated() {
  fprintf(stderr, "%s: file does not contain a complete Mach-O header\n",
          filename_.c_str());
}

void Reader::Reporter::LoadCommandRegionTruncated() {
  fprintf(stderr, "%s: file too short to hold load command region"
          " given in Mach-O header\n", filename_.c_str());
}

void Reader::Reporter::LoadCommandsOverrun(size_t claimed, size_t i,
                                           LoadCommandType type) {
  fprintf(stderr, "%s: file's header claims there are %ld"
          " load commands, but load command #%ld",
          filename_.c_str(), claimed, i);
  if (type) fprintf(stderr, ", of type %d,", type);
  fprintf(stderr, " extends beyond the end of the load command region\n");
}

void Reader::Reporter::LoadCommandTooShort(size_t i, LoadCommandType type) {
  fprintf(stderr, "%s: the contents of load command #%ld, of type %d,"
          " extend beyond the size given in the load command's header\n",
          filename_.c_str(), i, type);
}

void Reader::Reporter::SectionsMissing(const string &name) {
  fprintf(stderr, "%s: the load command for segment '%s'"
          " is too short to hold the section headers it claims to have\n",
          filename_.c_str(), name.c_str());
}

void Reader::Reporter::MisplacedSegmentData(const string &name) {
  fprintf(stderr, "%s: the segment '%s' claims its contents lie beyond"
          " the end of the file\n", filename_.c_str(), name.c_str());
}

void Reader::Reporter::MisplacedSectionData(const string &section,
                                            const string &segment) {
  fprintf(stderr, "%s: the section '%s' in segment '%s'"
          " claims its contents lie outside the segment's contents\n",
          filename_.c_str(), section.c_str(), segment.c_str());
}

void Reader::Reporter::MisplacedSymbolTable() {
  fprintf(stderr, "%s: the LC_SYMTAB load command claims that the symbol"
          " table's contents are located beyond the end of the file\n",
          filename_.c_str());
}

void Reader::Reporter::UnsupportedCPUType(cpu_type_t cpu_type) {
  fprintf(stderr, "%s: CPU type %d is not supported\n",
          filename_.c_str(), cpu_type);
}

bool Reader::Read(const uint8_t *buffer,
                  size_t size,
                  cpu_type_t expected_cpu_type,
                  cpu_subtype_t expected_cpu_subtype) {
  assert(!buffer_.start);
  buffer_.start = buffer;
  buffer_.end = buffer + size;
  ByteCursor cursor(&buffer_, true);
  uint32_t magic;
  if (!(cursor >> magic)) {
    reporter_->HeaderTruncated();
    return false;
  }

  if (expected_cpu_type != CPU_TYPE_ANY) {
    uint32_t expected_magic;
    
    switch (expected_cpu_type) {
      case CPU_TYPE_ARM:
      case CPU_TYPE_I386:
        expected_magic = MH_CIGAM;
        break;
      case CPU_TYPE_POWERPC:
        expected_magic = MH_MAGIC;
        break;
      case CPU_TYPE_X86_64:
        expected_magic = MH_CIGAM_64;
        break;
      case CPU_TYPE_POWERPC64:
        expected_magic = MH_MAGIC_64;
        break;
      default:
        reporter_->UnsupportedCPUType(expected_cpu_type);
        return false;
    }

    if (expected_magic != magic) {
      reporter_->BadHeader();
      return false;
    }
  }

  
  
  switch (magic) {
    case MH_MAGIC:    big_endian_ = true;  bits_64_ = false; break;
    case MH_CIGAM:    big_endian_ = false; bits_64_ = false; break;
    case MH_MAGIC_64: big_endian_ = true;  bits_64_ = true;  break;
    case MH_CIGAM_64: big_endian_ = false; bits_64_ = true;  break;
    default:
      reporter_->BadHeader();
      return false;
  }
  cursor.set_big_endian(big_endian_);
  uint32_t commands_size, reserved;
  cursor >> cpu_type_ >> cpu_subtype_ >> file_type_ >> load_command_count_
         >> commands_size >> flags_;
  if (bits_64_)
    cursor >> reserved;
  if (!cursor) {
    reporter_->HeaderTruncated();
    return false;
  }

  if (expected_cpu_type != CPU_TYPE_ANY &&
      (expected_cpu_type != cpu_type_ ||
       expected_cpu_subtype != cpu_subtype_)) {
    reporter_->CPUTypeMismatch(cpu_type_, cpu_subtype_,
                              expected_cpu_type, expected_cpu_subtype);
    return false;
  }

  cursor
      .PointTo(&load_commands_.start, commands_size)
      .PointTo(&load_commands_.end, 0);
  if (!cursor) {
    reporter_->LoadCommandRegionTruncated();
    return false;
  }

  return true;
}

bool Reader::WalkLoadCommands(Reader::LoadCommandHandler *handler) const {
  ByteCursor list_cursor(&load_commands_, big_endian_);

  for (size_t index = 0; index < load_command_count_; ++index) {
    
    
    
    
    ByteBuffer command(list_cursor.here(), list_cursor.Available());
    ByteCursor cursor(&command, big_endian_);
    
    
    uint32_t type, size;
    if (!(cursor >> type)) {
      reporter_->LoadCommandsOverrun(load_command_count_, index, 0);
      return false;
    }
    if (!(cursor >> size) || size > command.Size()) {
      reporter_->LoadCommandsOverrun(load_command_count_, index, type);
      return false;
    }

    
    
    command.end = command.start + size;

    switch (type) {
      case LC_SEGMENT:
      case LC_SEGMENT_64: {
        Segment segment;
        segment.bits_64 = (type == LC_SEGMENT_64);
        size_t word_size = segment.bits_64 ? 8 : 4;
        cursor.CString(&segment.name, 16);
        size_t file_offset, file_size;
        cursor
            .Read(word_size, false, &segment.vmaddr)
            .Read(word_size, false, &segment.vmsize)
            .Read(word_size, false, &file_offset)
            .Read(word_size, false, &file_size);
        cursor >> segment.maxprot
               >> segment.initprot
               >> segment.nsects
               >> segment.flags;
        if (!cursor) {
          reporter_->LoadCommandTooShort(index, type);
          return false;
        }
        if (file_offset > buffer_.Size() ||
            file_size > buffer_.Size() - file_offset) {
          reporter_->MisplacedSegmentData(segment.name);
          return false;
        }
        
        
        
        
        if (file_offset == 0 && file_size == 0) {
          segment.contents.start = segment.contents.end = NULL;
        } else {
          segment.contents.start = buffer_.start + file_offset;
          segment.contents.end = segment.contents.start + file_size;
        }
        
        segment.section_list.start = cursor.here();
        segment.section_list.end = command.end;

        if (!handler->SegmentCommand(segment))
          return false;
        break;
      }

      case LC_SYMTAB: {
        uint32_t symoff, nsyms, stroff, strsize;
        cursor >> symoff >> nsyms >> stroff >> strsize;
        if (!cursor) {
          reporter_->LoadCommandTooShort(index, type);
          return false;
        }
        
        
        
        size_t symbol_size = bits_64_ ? 16 : 12;
        
        size_t symbols_size = nsyms * symbol_size;
        if (symoff > buffer_.Size() || symbols_size > buffer_.Size() - symoff ||
            stroff > buffer_.Size() || strsize > buffer_.Size() - stroff) {
          reporter_->MisplacedSymbolTable();
          return false;
        }
        ByteBuffer entries(buffer_.start + symoff, symbols_size);
        ByteBuffer names(buffer_.start + stroff, strsize);
        if (!handler->SymtabCommand(entries, names))
          return false;
        break;
      }
      
      default: {
        if (!handler->UnknownCommand(type, command))
          return false;
        break;
      }
    }

    list_cursor.set_here(command.end);
  }

  return true;
}


class Reader::SegmentFinder : public LoadCommandHandler {
 public:
  
  
  SegmentFinder(const string &name, Segment *segment) 
      : name_(name), segment_(segment), found_() { }

  
  bool found() const { return found_; }

  bool SegmentCommand(const Segment &segment) {
    if (segment.name == name_) {
      *segment_ = segment;
      found_ = true;
      return false;
    }
    return true;
  }

 private:
  
  const string &name_;

  
  Segment *segment_;

  
  bool found_;
};

bool Reader::FindSegment(const string &name, Segment *segment) const {
  SegmentFinder finder(name, segment);
  WalkLoadCommands(&finder);
  return finder.found();
}

bool Reader::WalkSegmentSections(const Segment &segment,
                                 SectionHandler *handler) const {
  size_t word_size = segment.bits_64 ? 8 : 4;
  ByteCursor cursor(&segment.section_list, big_endian_);

  for (size_t i = 0; i < segment.nsects; i++) {
    Section section;
    section.bits_64 = segment.bits_64;
    uint64_t size;
    uint32_t offset, dummy32;
    cursor
        .CString(&section.section_name, 16)
        .CString(&section.segment_name, 16)
        .Read(word_size, false, &section.address)
        .Read(word_size, false, &size)
        >> offset
        >> section.align
        >> dummy32
        >> dummy32
        >> section.flags
        >> dummy32
        >> dummy32;
    if (section.bits_64)
      cursor >> dummy32;
    if (!cursor) {
      reporter_->SectionsMissing(segment.name);
      return false;
    }
    if ((section.flags & SECTION_TYPE) == S_ZEROFILL) {
      
      section.contents.start = section.contents.end = NULL;
    } else if (segment.contents.start == NULL && 
               segment.contents.end == NULL) {
      
      
      
      
      
      
      section.contents.start = section.contents.end = NULL;
    } else {
      if (offset < size_t(segment.contents.start - buffer_.start) ||
          offset > size_t(segment.contents.end - buffer_.start) ||
          size > size_t(segment.contents.end - buffer_.start - offset)) {
        reporter_->MisplacedSectionData(section.section_name,
                                        section.segment_name);
        return false;
      }
      section.contents.start = buffer_.start + offset;
      section.contents.end = section.contents.start + size;
    }
    if (!handler->HandleSection(section))
      return false;
  }
  return true;
}



class Reader::SectionMapper: public SectionHandler {
 public:
  
  
  SectionMapper(SectionMap *map) : map_(map) { }
  bool HandleSection(const Section &section) {
    (*map_)[section.section_name] = section;
    return true;
  }
 private:
  
  SectionMap *map_;
};

bool Reader::MapSegmentSections(const Segment &segment,
                                SectionMap *section_map) const {
  section_map->clear();
  SectionMapper mapper(section_map);
  return WalkSegmentSections(segment, &mapper);
}

}  
}  
