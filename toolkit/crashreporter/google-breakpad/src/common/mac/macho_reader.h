


































#ifndef BREAKPAD_COMMON_MAC_MACHO_READER_H_
#define BREAKPAD_COMMON_MAC_MACHO_READER_H_

#include <mach-o/loader.h>
#include <mach-o/fat.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>

#include <map>
#include <string>
#include <vector>

#include "common/byte_cursor.h"

namespace google_breakpad {
namespace mach_o {

using std::map;
using std::string;
using std::vector;





typedef uint32_t Magic;
typedef uint32_t FileType;
typedef uint32_t FileFlags;
typedef uint32_t LoadCommandType;
typedef uint32_t SegmentFlags;
typedef uint32_t SectionFlags;




class FatReader {
 public:

  
  
  class Reporter {
   public:
    
    explicit Reporter(const string &filename) : filename_(filename) { }

    virtual ~Reporter() { }

    
    
    virtual void BadHeader();

    
    
    virtual void MisplacedObjectFile();

    
    
    
    virtual void TooShort();
  
   private:
    
    string filename_;
  };

  
  explicit FatReader(Reporter *reporter) : reporter_(reporter) { }
  
  
  
  
  
  
  
  
  bool Read(const uint8_t *buffer, size_t size);

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  const struct fat_arch *object_files(size_t *count) const { 
    *count = object_files_.size();
    if (object_files_.size() > 0)
      return &object_files_[0];
    return NULL;
  }

 private:
  
  Reporter *reporter_;

  
  
  ByteBuffer buffer_;

  
  Magic magic_;

  
  
  vector<struct fat_arch> object_files_;
};



struct Segment {
  
  

  ByteBuffer section_list;    
  ByteBuffer contents;        

  
  string name;

  
  
  uint64_t vmaddr;

  
  
  
  
  uint64_t vmsize;

  
  uint32_t maxprot;
  uint32_t initprot;
  
  
  uint32_t nsects;

  
  uint32_t flags;

  
  bool bits_64;
};



struct Section {
  
  
  ByteBuffer contents;

  
  string section_name;  
  
  string segment_name;  

  
  
  
  uint64_t address;

  
  
  uint32_t align;

  
  uint32_t flags;

  

  
  bool bits_64;
};


typedef map<string, Section> SectionMap;






class Reader {
 public:

  
  
  
  class Reporter {
   public:
    
    explicit Reporter(const string &filename) : filename_(filename) { }
    virtual ~Reporter() { }

    
    

    
    
    
    virtual void BadHeader();

    
    
    
    virtual void CPUTypeMismatch(cpu_type_t cpu_type,
                                 cpu_subtype_t cpu_subtype,
                                 cpu_type_t expected_cpu_type,
                                 cpu_subtype_t expected_cpu_subtype);

    
    
    
    virtual void HeaderTruncated();

    
    
    virtual void LoadCommandRegionTruncated();

    
    
    
    
    virtual void LoadCommandsOverrun(size_t claimed, size_t i,
                                     LoadCommandType type);

    
    
    virtual void LoadCommandTooShort(size_t i, LoadCommandType type);

    
    
    
    virtual void SectionsMissing(const string &name);

    
    
    virtual void MisplacedSegmentData(const string &name);

    
    
    virtual void MisplacedSectionData(const string &section,
                                      const string &segment);

    
    
    virtual void MisplacedSymbolTable();

    
    
    virtual void UnsupportedCPUType(cpu_type_t cpu_type);

   private:
    string filename_;
  };

  
  
  
  class SectionHandler {
   public:
    virtual ~SectionHandler() { }

    
    
    
    virtual bool HandleSection(const Section &section) = 0;
  };

  
  class LoadCommandHandler {
   public:
    LoadCommandHandler() { }
    virtual ~LoadCommandHandler() { }

    
    
    
    
    
    
    
    
    
    
    

    
    
    
    
    virtual bool UnknownCommand(LoadCommandType type,
                                const ByteBuffer &contents) {
      return true;
    }

    
    
    virtual bool SegmentCommand(const Segment &segment) {
      return true;
    }

    
    
    virtual bool SymtabCommand(const ByteBuffer &entries,
                               const ByteBuffer &names) {
      return true;
    }

    
  };

  
  explicit Reader(Reporter *reporter)
      : reporter_(reporter) { }

  
  
  
  
  
  
  bool Read(const uint8_t *buffer,
            size_t size,
            cpu_type_t expected_cpu_type,
            cpu_subtype_t expected_cpu_subtype);
  bool Read(const ByteBuffer &buffer,
            cpu_type_t expected_cpu_type,
            cpu_subtype_t expected_cpu_subtype) {
    return Read(buffer.start,
                buffer.Size(),
                expected_cpu_type,
                expected_cpu_subtype); 
  }

  
  cpu_type_t    cpu_type()    const { return cpu_type_; }
  cpu_subtype_t cpu_subtype() const { return cpu_subtype_; }
  FileType      file_type()   const { return file_type_; }
  FileFlags     flags()       const { return flags_; }

  
  
  bool bits_64() const { return bits_64_; }

  
  
  bool big_endian() const { return big_endian_; }

  
  
  
  
  bool WalkLoadCommands(LoadCommandHandler *handler) const;

  
  
  
  
  bool FindSegment(const string &name, Segment *segment) const;

  
  
  
  bool WalkSegmentSections(const Segment &segment, SectionHandler *handler)
    const;

  
  
  
  
  bool MapSegmentSections(const Segment &segment, SectionMap *section_map)
    const;

 private:
  
  class SegmentFinder;
  class SectionMapper;

  
  Reporter *reporter_;

  
  
  ByteBuffer buffer_;

  
  bool big_endian_;

  
  bool bits_64_;

  
  cpu_type_t cpu_type_;        
  cpu_subtype_t cpu_subtype_;  

  
  FileType file_type_;         

  
  ByteBuffer load_commands_;

  
  uint32_t load_command_count_;  

  
  FileFlags flags_;
};

}  
}  

#endif  
