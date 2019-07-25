

































#include "common/linux/dump_symbols.h"

#include <assert.h>
#include <elf.h>
#include <errno.h>
#include <fcntl.h>
#include <link.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#include <set>
#include <string>
#include <utility>
#include <vector>

#include "common/dwarf/bytereader-inl.h"
#include "common/dwarf/dwarf2diehandler.h"
#include "common/dwarf_cfi_to_module.h"
#include "common/dwarf_cu_to_module.h"
#include "common/dwarf_line_to_module.h"
#include "common/linux/file_id.h"
#include "common/module.h"
#include "common/stabs_reader.h"
#include "common/stabs_to_module.h"


namespace {

using google_breakpad::DwarfCFIToModule;
using google_breakpad::DwarfCUToModule;
using google_breakpad::DwarfLineToModule;
using google_breakpad::Module;
using google_breakpad::StabsToModule;






class FDWrapper {
 public:
  explicit FDWrapper(int fd) :
    fd_(fd) {}
  ~FDWrapper() {
    if (fd_ != -1)
      close(fd_);
  }
  int get() {
    return fd_;
  }
  int release() {
    int fd = fd_;
    fd_ = -1;
    return fd;
  }
 private:
  int fd_;
};






class MmapWrapper {
 public:
  MmapWrapper() : is_set_(false) {}
  ~MmapWrapper() {
    assert(is_set_);
    if (base_ != NULL) {
      assert(size_ > 0);
      munmap(base_, size_);
    }
  }
  void set(void *mapped_address, size_t mapped_size) {
    is_set_ = true;
    base_ = mapped_address;
    size_ = mapped_size;
  }
  void release() {
    assert(is_set_);
    base_ = NULL;
    size_ = 0;
  }

 private:
  bool is_set_;
  void *base_;
  size_t size_;
};




static void FixAddress(void *obj_base) {
  ElfW(Addr) base = reinterpret_cast<ElfW(Addr)>(obj_base);
  ElfW(Ehdr) *elf_header = static_cast<ElfW(Ehdr) *>(obj_base);
  elf_header->e_phoff += base;
  elf_header->e_shoff += base;
  ElfW(Shdr) *sections = reinterpret_cast<ElfW(Shdr) *>(elf_header->e_shoff);
  for (int i = 0; i < elf_header->e_shnum; ++i)
    sections[i].sh_offset += base;
}


static ElfW(Addr) GetLoadingAddress(const ElfW(Phdr) *program_headers,
                                    int nheader) {
  for (int i = 0; i < nheader; ++i) {
    const ElfW(Phdr) &header = program_headers[i];
    
    if (header.p_type == PT_LOAD &&
        header.p_offset == 0)
      return header.p_vaddr;
  }
  
  return 0;
}

static bool IsValidElf(const ElfW(Ehdr) *elf_header) {
  return memcmp(elf_header, ELFMAG, SELFMAG) == 0;
}

static const ElfW(Shdr) *FindSectionByName(const char *name,
                                           const ElfW(Shdr) *sections,
                                           const ElfW(Shdr) *section_names,
                                           int nsection) {
  assert(name != NULL);
  assert(sections != NULL);
  assert(nsection > 0);

  int name_len = strlen(name);
  if (name_len == 0)
    return NULL;

  
  
  const char *names_end =
    reinterpret_cast<char*>(section_names->sh_offset + section_names->sh_size);

  for (int i = 0; i < nsection; ++i) {
    const char *section_name =
      reinterpret_cast<char*>(section_names->sh_offset + sections[i].sh_name);
    if (names_end - section_name >= name_len + 1 &&
        strcmp(name, section_name) == 0) {
      if (sections[i].sh_type == SHT_NOBITS) {
        fprintf(stderr,
                "Section %s found, but ignored because type=SHT_NOBITS.\n",
                name);
        return NULL;
      }
      return sections + i;
    }
  }
  return NULL;
}

static bool LoadStabs(const ElfW(Ehdr) *elf_header,
                      const ElfW(Shdr) *stab_section,
                      const ElfW(Shdr) *stabstr_section,
                      const bool big_endian,
                      Module *module) {
  
  StabsToModule handler(module);
  
  
  
  
  uint8_t *stabs = reinterpret_cast<uint8_t *>(stab_section->sh_offset);
  uint8_t *stabstr = reinterpret_cast<uint8_t *>(stabstr_section->sh_offset);
  google_breakpad::StabsReader reader(stabs, stab_section->sh_size,
                                      stabstr, stabstr_section->sh_size,
                                      big_endian, 4, true, &handler);
  
  if (!reader.Process())
    return false;
  handler.Finalize();
  return true;
}




class DumperLineToModule: public DwarfCUToModule::LineToModuleFunctor {
 public:
  
  explicit DumperLineToModule(dwarf2reader::ByteReader *byte_reader)
      : byte_reader_(byte_reader) { }
  void operator()(const char *program, uint64 length,
                  Module *module, vector<Module::Line> *lines) {
    DwarfLineToModule handler(module, lines);
    dwarf2reader::LineInfo parser(program, length, byte_reader_, &handler);
    parser.Start();
  }
 private:
  dwarf2reader::ByteReader *byte_reader_;
};

static bool LoadDwarf(const string &dwarf_filename,
                      const ElfW(Ehdr) *elf_header,
                      const bool big_endian,
                      Module *module) {
  const dwarf2reader::Endianness endianness = big_endian ?
      dwarf2reader::ENDIANNESS_BIG : dwarf2reader::ENDIANNESS_LITTLE;
  dwarf2reader::ByteReader byte_reader(endianness);

  
  DwarfCUToModule::FileContext file_context(dwarf_filename, module);

  
  const ElfW(Shdr) *sections
      = reinterpret_cast<ElfW(Shdr) *>(elf_header->e_shoff);
  int num_sections = elf_header->e_shnum;
  const ElfW(Shdr) *section_names = sections + elf_header->e_shstrndx;
  for (int i = 0; i < num_sections; i++) {
    const ElfW(Shdr) *section = &sections[i];
    string name = reinterpret_cast<const char *>(section_names->sh_offset
                                                 + section->sh_name);
    const char *contents = reinterpret_cast<const char *>(section->sh_offset);
    uint64 length = section->sh_size;
    file_context.section_map[name] = std::make_pair(contents, length);
  }

  
  DumperLineToModule line_to_module(&byte_reader);
  std::pair<const char *, uint64> debug_info_section
      = file_context.section_map[".debug_info"];
  
  
  assert(debug_info_section.first);
  uint64 debug_info_length = debug_info_section.second;
  for (uint64 offset = 0; offset < debug_info_length;) {
    
    
    DwarfCUToModule::WarningReporter reporter(dwarf_filename, offset);
    DwarfCUToModule root_handler(&file_context, &line_to_module, &reporter);
    
    dwarf2reader::DIEDispatcher die_dispatcher(&root_handler);
    
    dwarf2reader::CompilationUnit reader(file_context.section_map,
                                         offset,
                                         &byte_reader,
                                         &die_dispatcher);
    
    offset += reader.Start();
  }
  return true;
}






static bool DwarfCFIRegisterNames(const ElfW(Ehdr) *elf_header,
                                  vector<string> *register_names) {
  switch (elf_header->e_machine) {
    case EM_386:
      *register_names = DwarfCFIToModule::RegisterNames::I386();
      return true;
    case EM_ARM:
      *register_names = DwarfCFIToModule::RegisterNames::ARM();
      return true;
    case EM_X86_64:
      *register_names = DwarfCFIToModule::RegisterNames::X86_64();
      return true;
    default:
      return false;
  }
}

static bool LoadDwarfCFI(const string &dwarf_filename,
                         const ElfW(Ehdr) *elf_header,
                         const char *section_name,
                         const ElfW(Shdr) *section,
                         const bool eh_frame,
                         const ElfW(Shdr) *got_section,
                         const ElfW(Shdr) *text_section,
                         const bool big_endian,
                         Module *module) {
  
  
  vector<string> register_names;
  if (!DwarfCFIRegisterNames(elf_header, &register_names)) {
    fprintf(stderr, "%s: unrecognized ELF machine architecture '%d';"
            " cannot convert DWARF call frame information\n",
            dwarf_filename.c_str(), elf_header->e_machine);
    return false;
  }

  const dwarf2reader::Endianness endianness = big_endian ?
      dwarf2reader::ENDIANNESS_BIG : dwarf2reader::ENDIANNESS_LITTLE;

  
  const char *cfi = reinterpret_cast<const char *>(section->sh_offset);
  size_t cfi_size = section->sh_size;

  
  DwarfCFIToModule::Reporter module_reporter(dwarf_filename, section_name);
  DwarfCFIToModule handler(module, register_names, &module_reporter);
  dwarf2reader::ByteReader byte_reader(endianness);
  
  
  
  if (elf_header->e_ident[EI_CLASS] == ELFCLASS32)
    byte_reader.SetAddressSize(4);
  else if (elf_header->e_ident[EI_CLASS] == ELFCLASS64)
    byte_reader.SetAddressSize(8);
  else {
    fprintf(stderr, "%s: bad file class in ELF header: %d\n",
            dwarf_filename.c_str(), elf_header->e_ident[EI_CLASS]);
    return false;
  }
  
  
  byte_reader.SetCFIDataBase(section->sh_addr, cfi);
  if (got_section)
    byte_reader.SetDataBase(got_section->sh_addr);
  if (text_section)
    byte_reader.SetTextBase(text_section->sh_addr);

  dwarf2reader::CallFrameInfo::Reporter dwarf_reporter(dwarf_filename,
                                                       section_name);
  dwarf2reader::CallFrameInfo parser(cfi, cfi_size,
                                     &byte_reader, &handler, &dwarf_reporter,
                                     eh_frame);
  parser.Start();
  return true;
}

bool LoadELF(const std::string &obj_file, MmapWrapper* map_wrapper,
             ElfW(Ehdr) **elf_header) {
  int obj_fd = open(obj_file.c_str(), O_RDONLY);
  if (obj_fd < 0) {
    fprintf(stderr, "Failed to open ELF file '%s': %s\n",
            obj_file.c_str(), strerror(errno));
    return false;
  }
  FDWrapper obj_fd_wrapper(obj_fd);
  struct stat st;
  if (fstat(obj_fd, &st) != 0 && st.st_size <= 0) {
    fprintf(stderr, "Unable to fstat ELF file '%s': %s\n",
            obj_file.c_str(), strerror(errno));
    return false;
  }
  void *obj_base = mmap(NULL, st.st_size,
                        PROT_READ | PROT_WRITE, MAP_PRIVATE, obj_fd, 0);
  if (obj_base == MAP_FAILED) {
    fprintf(stderr, "Failed to mmap ELF file '%s': %s\n",
            obj_file.c_str(), strerror(errno));
    return false;
  }
  map_wrapper->set(obj_base, st.st_size);
  *elf_header = reinterpret_cast<ElfW(Ehdr) *>(obj_base);
  if (!IsValidElf(*elf_header)) {
    fprintf(stderr, "Not a valid ELF file: %s\n", obj_file.c_str());
    return false;
  }
  return true;
}


bool ElfEndianness(const ElfW(Ehdr) *elf_header, bool *big_endian) {
  if (elf_header->e_ident[EI_DATA] == ELFDATA2LSB) {
    *big_endian = false;
    return true;
  }
  if (elf_header->e_ident[EI_DATA] == ELFDATA2MSB) {
    *big_endian = true;
    return true;
  }

  fprintf(stderr, "bad data encoding in ELF header: %d\n",
          elf_header->e_ident[EI_DATA]);
  return false;
}



static std::string ReadDebugLink(const ElfW(Shdr) *debuglink_section,
                                 const std::string &obj_file,
                                 const std::string &debug_dir) {
  char *debuglink = reinterpret_cast<char *>(debuglink_section->sh_offset);
  size_t debuglink_len = strlen(debuglink) + 5;  
  debuglink_len = 4 * ((debuglink_len + 3) / 4);  

  
  if (debuglink_len != debuglink_section->sh_size) {
    fprintf(stderr, "Mismatched .gnu_debuglink string / section size: "
            "%zx %zx\n", debuglink_len, debuglink_section->sh_size);
    return "";
  }

  std::string debuglink_path = debug_dir + "/" + debuglink;
  int debuglink_fd = open(debuglink_path.c_str(), O_RDONLY);
  if (debuglink_fd < 0) {
    fprintf(stderr, "Failed to open debug ELF file '%s' for '%s': %s\n",
            debuglink_path.c_str(), obj_file.c_str(), strerror(errno));
    return "";
  }
  FDWrapper debuglink_fd_wrapper(debuglink_fd);
  
  

  return debuglink_path;
}








class LoadSymbolsInfo {
 public:
  explicit LoadSymbolsInfo(const std::string &dbg_dir) :
    debug_dir_(dbg_dir),
    has_loading_addr_(false) {}

  
  
  void LoadedSection(const std::string &section) {
    if (loaded_sections_.count(section) == 0) {
      loaded_sections_.insert(section);
    } else {
      fprintf(stderr, "Section %s has already been loaded.\n",
              section.c_str());
    }
  }

  
  
  void set_loading_addr(ElfW(Addr) addr, const std::string &filename) {
    if (!has_loading_addr_) {
      loading_addr_ = addr;
      loaded_file_ = filename;
      return;
    }

    if (addr != loading_addr_) {
      fprintf(stderr,
              "ELF file '%s' and debug ELF file '%s' "
              "have different load addresses.\n",
              loaded_file_.c_str(), filename.c_str());
      assert(false);
    }
  }

  
  const std::string &debug_dir() const {
    return debug_dir_;
  }

  std::string debuglink_file() const {
    return debuglink_file_;
  }
  void set_debuglink_file(std::string file) {
    debuglink_file_ = file;
  }

 private:
  const std::string &debug_dir_;  

  std::string debuglink_file_;  

  bool has_loading_addr_;  

  ElfW(Addr) loading_addr_;  
                             

  std::string loaded_file_;  
                             

  std::set<std::string> loaded_sections_;  
                                           
};

static bool LoadSymbols(const std::string &obj_file,
                        const bool big_endian,
                        ElfW(Ehdr) *elf_header,
                        const bool read_gnu_debug_link,
                        LoadSymbolsInfo *info,
                        Module *module) {
  
  FixAddress(elf_header);
  ElfW(Addr) loading_addr = GetLoadingAddress(
      reinterpret_cast<ElfW(Phdr) *>(elf_header->e_phoff),
      elf_header->e_phnum);
  module->SetLoadAddress(loading_addr);
  info->set_loading_addr(loading_addr, obj_file);

  const ElfW(Shdr) *sections =
      reinterpret_cast<ElfW(Shdr) *>(elf_header->e_shoff);
  const ElfW(Shdr) *section_names = sections + elf_header->e_shstrndx;
  bool found_debug_info_section = false;

  
  const ElfW(Shdr) *stab_section
      = FindSectionByName(".stab", sections, section_names,
                          elf_header->e_shnum);
  if (stab_section) {
    const ElfW(Shdr) *stabstr_section = stab_section->sh_link + sections;
    if (stabstr_section) {
      found_debug_info_section = true;
      info->LoadedSection(".stab");
      if (!LoadStabs(elf_header, stab_section, stabstr_section, big_endian,
                     module)) {
        fprintf(stderr, "%s: \".stab\" section found, but failed to load STABS"
                " debugging information\n", obj_file.c_str());
      }
    }
  }

  
  const ElfW(Shdr) *dwarf_section
      = FindSectionByName(".debug_info", sections, section_names,
                          elf_header->e_shnum);
  if (dwarf_section) {
    found_debug_info_section = true;
    info->LoadedSection(".debug_info");
    if (!LoadDwarf(obj_file, elf_header, big_endian, module))
      fprintf(stderr, "%s: \".debug_info\" section found, but failed to load "
              "DWARF debugging information\n", obj_file.c_str());
  }

  
  
  const ElfW(Shdr) *dwarf_cfi_section =
      FindSectionByName(".debug_frame", sections, section_names,
                          elf_header->e_shnum);
  if (dwarf_cfi_section) {
    
    
    
    info->LoadedSection(".debug_frame");
    LoadDwarfCFI(obj_file, elf_header, ".debug_frame",
                 dwarf_cfi_section, false, 0, 0, big_endian, module);
  }

  
  
  const ElfW(Shdr) *eh_frame_section =
      FindSectionByName(".eh_frame", sections, section_names,
                        elf_header->e_shnum);
  if (eh_frame_section) {
    
    
    const ElfW(Shdr) *got_section =
      FindSectionByName(".got", sections, section_names, elf_header->e_shnum);
    const ElfW(Shdr) *text_section =
      FindSectionByName(".text", sections, section_names,
                        elf_header->e_shnum);
    info->LoadedSection(".eh_frame");
    
    LoadDwarfCFI(obj_file, elf_header, ".eh_frame", eh_frame_section, true,
                 got_section, text_section, big_endian, module);
  }

  if (!found_debug_info_section) {
    fprintf(stderr, "%s: file contains no debugging information"
            " (no \".stab\" or \".debug_info\" sections)\n",
            obj_file.c_str());

    
    if (read_gnu_debug_link) {
      const ElfW(Shdr) *gnu_debuglink_section
          = FindSectionByName(".gnu_debuglink", sections, section_names,
                              elf_header->e_shnum);
      if (gnu_debuglink_section) {
        if (!info->debug_dir().empty()) {
          std::string debuglink_file =
              ReadDebugLink(gnu_debuglink_section, obj_file, info->debug_dir());
          info->set_debuglink_file(debuglink_file);
        } else {
          fprintf(stderr, ".gnu_debuglink section found in '%s', "
                  "but no debug path specified.\n", obj_file.c_str());
        }
      } else {
        fprintf(stderr, "%s does not contain a .gnu_debuglink section.\n",
                obj_file.c_str());
      }
    }
    return false;
  }

  return true;
}



const char *ElfArchitecture(const ElfW(Ehdr) *elf_header) {
  ElfW(Half) arch = elf_header->e_machine;
  switch (arch) {
    case EM_386:        return "x86";
    case EM_ARM:        return "arm";
    case EM_MIPS:       return "mips";
    case EM_PPC64:      return "ppc64";
    case EM_PPC:        return "ppc";
    case EM_S390:       return "s390";
    case EM_SPARC:      return "sparc";
    case EM_SPARCV9:    return "sparcv9";
    case EM_X86_64:     return "x86_64";
    default: return NULL;
  }
}



std::string FormatIdentifier(unsigned char identifier[16]) {
  char identifier_str[40];
  google_breakpad::FileID::ConvertIdentifierToString(
      identifier,
      identifier_str,
      sizeof(identifier_str));
  std::string id_no_dash;
  for (int i = 0; identifier_str[i] != '\0'; ++i)
    if (identifier_str[i] != '-')
      id_no_dash += identifier_str[i];
  
  
  
  
  id_no_dash += '0';
  return id_no_dash;
}



std::string BaseFileName(const std::string &filename) {
  
  char *c_filename = strdup(filename.c_str());
  std::string base = basename(c_filename);
  free(c_filename);
  return base;
}

}  

namespace google_breakpad {

bool WriteSymbolFile(const std::string &obj_file,
                     const std::string &debug_dir, FILE *sym_file) {
  MmapWrapper map_wrapper;
  ElfW(Ehdr) *elf_header = NULL;
  if (!LoadELF(obj_file, &map_wrapper, &elf_header))
    return false;

  unsigned char identifier[16];
  google_breakpad::FileID file_id(obj_file.c_str());
  if (!file_id.ElfFileIdentifierFromMappedFile(elf_header, identifier)) {
    fprintf(stderr, "%s: unable to generate file identifier\n",
            obj_file.c_str());
    return false;
  }

  const char *architecture = ElfArchitecture(elf_header);
  if (!architecture) {
    fprintf(stderr, "%s: unrecognized ELF machine architecture: %d\n",
            obj_file.c_str(), elf_header->e_machine);
    return false;
  }

  
  bool big_endian;
  if (!ElfEndianness(elf_header, &big_endian))
    return false;

  std::string name = BaseFileName(obj_file);
  std::string os = "Linux";
  std::string id = FormatIdentifier(identifier);

  LoadSymbolsInfo info(debug_dir);
  Module module(name, os, architecture, id);
  if (!LoadSymbols(obj_file, big_endian, elf_header, true, &info, &module)) {
    const std::string debuglink_file = info.debuglink_file();
    if (debuglink_file.empty())
      return false;

    
    fprintf(stderr, "Found debugging info in %s\n", debuglink_file.c_str());
    MmapWrapper debug_map_wrapper;
    ElfW(Ehdr) *debug_elf_header = NULL;
    if (!LoadELF(debuglink_file, &debug_map_wrapper, &debug_elf_header))
      return false;
    
    const char *debug_architecture = ElfArchitecture(debug_elf_header);
    if (!debug_architecture) {
      fprintf(stderr, "%s: unrecognized ELF machine architecture: %d\n",
              debuglink_file.c_str(), debug_elf_header->e_machine);
      return false;
    }
    if (strcmp(architecture, debug_architecture)) {
      fprintf(stderr, "%s with ELF machine architecture %s does not match "
              "%s with ELF architecture %s\n",
              debuglink_file.c_str(), debug_architecture,
              obj_file.c_str(), architecture);
      return false;
    }

    bool debug_big_endian;
    if (!ElfEndianness(debug_elf_header, &debug_big_endian))
      return false;
    if (debug_big_endian != big_endian) {
      fprintf(stderr, "%s and %s does not match in endianness\n",
              obj_file.c_str(), debuglink_file.c_str());
      return false;
    }

    if (!LoadSymbols(debuglink_file, debug_big_endian, debug_elf_header,
                     false, &info, &module)) {
      return false;
    }
  }
  if (!module.Write(sym_file))
    return false;

  return true;
}

}  
