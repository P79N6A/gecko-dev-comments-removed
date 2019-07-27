

































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

#include <iostream>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "common/arm_ex_reader.h"
#include "common/dwarf/bytereader-inl.h"
#include "common/dwarf/dwarf2diehandler.h"
#include "common/dwarf_cfi_to_module.h"
#include "common/dwarf_cu_to_module.h"
#include "common/dwarf_line_to_module.h"
#include "common/linux/elfutils.h"
#include "common/linux/elfutils-inl.h"
#include "common/linux/elf_symbols_to_module.h"
#include "common/linux/file_id.h"
#include "common/module.h"
#include "common/scoped_ptr.h"
#ifndef NO_STABS_SUPPORT
#include "common/stabs_reader.h"
#include "common/stabs_to_module.h"
#endif
#include "common/using_std_string.h"
#include "common/logging.h"

#ifndef SHT_ARM_EXIDX

# define SHT_ARM_EXIDX (SHT_LOPROC + 1)
#endif


namespace {

using google_breakpad::DwarfCFIToModule;
using google_breakpad::DwarfCUToModule;
using google_breakpad::DwarfLineToModule;
using google_breakpad::ElfClass;
using google_breakpad::ElfClass32;
using google_breakpad::ElfClass64;
using google_breakpad::FindElfSectionByName;
using google_breakpad::GetOffset;
using google_breakpad::IsValidElf;
using google_breakpad::Module;
#ifndef NO_STABS_SUPPORT
using google_breakpad::StabsToModule;
#endif
using google_breakpad::UniqueString;
using google_breakpad::scoped_ptr;






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
    if (is_set_ && base_ != NULL) {
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
    is_set_ = false;
    base_ = NULL;
    size_ = 0;
  }

 private:
  bool is_set_;
  void *base_;
  size_t size_;
};


template<typename ElfClass>
typename ElfClass::Addr GetLoadingAddress(
    const typename ElfClass::Phdr* program_headers,
    int nheader) {
  typedef typename ElfClass::Phdr Phdr;

  for (int i = 0; i < nheader; ++i) {
    const Phdr& header = program_headers[i];
    
    if (header.p_type == PT_LOAD &&
        header.p_offset == 0)
      return header.p_vaddr;
  }
  
  return 0;
}

#ifndef NO_STABS_SUPPORT
template<typename ElfClass>
bool LoadStabs(const typename ElfClass::Ehdr* elf_header,
               const typename ElfClass::Shdr* stab_section,
               const typename ElfClass::Shdr* stabstr_section,
               const bool big_endian,
               Module* module) {
  
  StabsToModule handler(module);
  
  
  
  
  const uint8_t* stabs =
      GetOffset<ElfClass, uint8_t>(elf_header, stab_section->sh_offset);
  const uint8_t* stabstr =
      GetOffset<ElfClass, uint8_t>(elf_header, stabstr_section->sh_offset);
  google_breakpad::StabsReader reader(stabs, stab_section->sh_size,
                                      stabstr, stabstr_section->sh_size,
                                      big_endian, 4, true, &handler);
  
  if (!reader.Process())
    return false;
  handler.Finalize();
  return true;
}
#endif  




class DumperLineToModule: public DwarfCUToModule::LineToModuleHandler {
 public:
  
  explicit DumperLineToModule(dwarf2reader::ByteReader *byte_reader)
      : byte_reader_(byte_reader) { }
  void StartCompilationUnit(const string& compilation_dir) {
    compilation_dir_ = compilation_dir;
  }
  void ReadProgram(const char *program, uint64 length,
                   Module *module, std::vector<Module::Line> *lines) {
    DwarfLineToModule handler(module, compilation_dir_, lines);
    dwarf2reader::LineInfo parser(program, length, byte_reader_, &handler);
    parser.Start();
  }
 private:
  string compilation_dir_;
  dwarf2reader::ByteReader *byte_reader_;
};

template<typename ElfClass>
bool LoadDwarf(const string& dwarf_filename,
               const typename ElfClass::Ehdr* elf_header,
               const bool big_endian,
               Module* module) {
  typedef typename ElfClass::Shdr Shdr;

  const dwarf2reader::Endianness endianness = big_endian ?
      dwarf2reader::ENDIANNESS_BIG : dwarf2reader::ENDIANNESS_LITTLE;
  dwarf2reader::ByteReader byte_reader(endianness);

  
  DwarfCUToModule::FileContext file_context(dwarf_filename, module);

  
  const Shdr* sections =
      GetOffset<ElfClass, Shdr>(elf_header, elf_header->e_shoff);
  int num_sections = elf_header->e_shnum;
  const Shdr* section_names = sections + elf_header->e_shstrndx;
  for (int i = 0; i < num_sections; i++) {
    const Shdr* section = &sections[i];
    string name = GetOffset<ElfClass, char>(elf_header,
                                            section_names->sh_offset) +
                  section->sh_name;
    const char* contents = GetOffset<ElfClass, char>(elf_header,
                                                     section->sh_offset);
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






template<typename ElfClass>
bool DwarfCFIRegisterNames(const typename ElfClass::Ehdr* elf_header,
                           std::vector<const UniqueString*>* register_names) {
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

template<typename ElfClass>
bool LoadDwarfCFI(const string& dwarf_filename,
                  const typename ElfClass::Ehdr* elf_header,
                  const char* section_name,
                  const typename ElfClass::Shdr* section,
                  const bool eh_frame,
                  const typename ElfClass::Shdr* got_section,
                  const typename ElfClass::Shdr* text_section,
                  const bool big_endian,
                  Module* module) {
  
  
  std::vector<const UniqueString*> register_names;
  if (!DwarfCFIRegisterNames<ElfClass>(elf_header, &register_names)) {
    fprintf(stderr, "%s: unrecognized ELF machine architecture '%d';"
            " cannot convert DWARF call frame information\n",
            dwarf_filename.c_str(), elf_header->e_machine);
    return false;
  }

  const dwarf2reader::Endianness endianness = big_endian ?
      dwarf2reader::ENDIANNESS_BIG : dwarf2reader::ENDIANNESS_LITTLE;

  
  const char* cfi =
      GetOffset<ElfClass, char>(elf_header, section->sh_offset);
  size_t cfi_size = section->sh_size;

  
  DwarfCFIToModule::Reporter module_reporter(dwarf_filename, section_name);
  DwarfCFIToModule handler(module, register_names, &module_reporter);
  dwarf2reader::ByteReader byte_reader(endianness);

  byte_reader.SetAddressSize(ElfClass::kAddrSize);

  
  
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

template<typename ElfClass>
bool LoadARMexidx(const typename ElfClass::Ehdr* elf_header,
                  const typename ElfClass::Shdr* exidx_section,
                  const typename ElfClass::Shdr* extab_section,
                  uint32_t loading_addr,
                  Module* module) {
  
  
  
  
  
  
  const char *exidx_img
    = GetOffset<ElfClass, char>(elf_header, exidx_section->sh_offset);
  size_t exidx_size = exidx_section->sh_size;
  const char *extab_img
    = GetOffset<ElfClass, char>(elf_header, extab_section->sh_offset);
  size_t extab_size = extab_section->sh_size;

  
  
  uint32_t exidx_text_last_svma = 0;
  int exidx_text_sno = exidx_section->sh_link;
  typedef typename ElfClass::Shdr Shdr;
  
  const Shdr* sections
    = GetOffset<ElfClass, Shdr>(elf_header, elf_header->e_shoff);
  const int num_sections = elf_header->e_shnum;
  if (exidx_text_sno >= 0 && exidx_text_sno < num_sections) {
    const Shdr* exidx_text_shdr = &sections[exidx_text_sno];
    if (exidx_text_shdr->sh_size > 0) {
      exidx_text_last_svma
        = exidx_text_shdr->sh_addr + exidx_text_shdr->sh_size - 1;
    }
  }

  arm_ex_to_module::ARMExToModule handler(module);
  arm_ex_reader::ExceptionTableInfo
    parser(exidx_img, exidx_size, extab_img, extab_size, exidx_text_last_svma,
           &handler,
           reinterpret_cast<const char*>(elf_header),
           loading_addr);
  parser.Start();
  return true;
}

bool LoadELF(const string& obj_file, MmapWrapper* map_wrapper,
             void** elf_header) {
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
  *elf_header = obj_base;
  if (!IsValidElf(*elf_header)) {
    fprintf(stderr, "Not a valid ELF file: %s\n", obj_file.c_str());
    return false;
  }
  return true;
}


template<typename ElfClass>
bool ElfEndianness(const typename ElfClass::Ehdr* elf_header,
                   bool* big_endian) {
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



template<typename ElfClass>
string ReadDebugLink(const char* debuglink,
                     size_t debuglink_size,
                     const string& obj_file,
                     const std::vector<string>& debug_dirs) {
  size_t debuglink_len = strlen(debuglink) + 5;  
  debuglink_len = 4 * ((debuglink_len + 3) / 4);  

  
  if (debuglink_len != debuglink_size) {
    fprintf(stderr, "Mismatched .gnu_debuglink string / section size: "
            "%zx %zx\n", debuglink_len, debuglink_size);
    return "";
  }

  bool found = false;
  int debuglink_fd = -1;
  string debuglink_path;
  std::vector<string>::const_iterator it;
  for (it = debug_dirs.begin(); it < debug_dirs.end(); ++it) {
    const string& debug_dir = *it;
    debuglink_path = debug_dir + "/" + debuglink;
    debuglink_fd = open(debuglink_path.c_str(), O_RDONLY);
    if (debuglink_fd >= 0) {
      found = true;
      break;
    }
  }

  if (!found) {
    fprintf(stderr, "Failed to find debug ELF file for '%s' after trying:\n",
            obj_file.c_str());
    for (it = debug_dirs.begin(); it < debug_dirs.end(); ++it) {
      const string debug_dir = *it;
      fprintf(stderr, "  %s/%s\n", debug_dir.c_str(), debuglink);
    }
    return "";
  }

  FDWrapper debuglink_fd_wrapper(debuglink_fd);
  
  

  return debuglink_path;
}








template<typename ElfClass>
class LoadSymbolsInfo {
 public:
  typedef typename ElfClass::Addr Addr;

  explicit LoadSymbolsInfo(const std::vector<string>& dbg_dirs) :
    debug_dirs_(dbg_dirs),
    has_loading_addr_(false) {}

  
  
  void LoadedSection(const string &section) {
    if (loaded_sections_.count(section) == 0) {
      loaded_sections_.insert(section);
    } else {
      fprintf(stderr, "Section %s has already been loaded.\n",
              section.c_str());
    }
  }

  
  
  void set_loading_addr(Addr addr, const string &filename) {
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

  
  const std::vector<string>& debug_dirs() const {
    return debug_dirs_;
  }

  string debuglink_file() const {
    return debuglink_file_;
  }
  void set_debuglink_file(string file) {
    debuglink_file_ = file;
  }

 private:
  const std::vector<string>& debug_dirs_; 
                                          

  string debuglink_file_;  

  bool has_loading_addr_;  

  Addr loading_addr_;  
                       

  string loaded_file_;  
                        

  std::set<string> loaded_sections_;  
                                      
};

template<typename ElfClass>
bool LoadSymbols(const string& obj_file,
                 const bool big_endian,
                 const typename ElfClass::Ehdr* elf_header,
                 const bool read_gnu_debug_link,
                 LoadSymbolsInfo<ElfClass>* info,
                 SymbolData symbol_data,
                 Module* module) {
  typedef typename ElfClass::Addr Addr;
  typedef typename ElfClass::Phdr Phdr;
  typedef typename ElfClass::Shdr Shdr;

  BPLOG(INFO) << "";
  BPLOG(INFO) << "LoadSymbols: BEGIN   " << obj_file;

  Addr loading_addr = GetLoadingAddress<ElfClass>(
      GetOffset<ElfClass, Phdr>(elf_header, elf_header->e_phoff),
      elf_header->e_phnum);
  module->SetLoadAddress(loading_addr);
  info->set_loading_addr(loading_addr, obj_file);

  const Shdr* sections =
      GetOffset<ElfClass, Shdr>(elf_header, elf_header->e_shoff);
  const Shdr* section_names = sections + elf_header->e_shstrndx;
  const char* names =
      GetOffset<ElfClass, char>(elf_header, section_names->sh_offset);
  const char *names_end = names + section_names->sh_size;
  bool found_debug_info_section = false;
  bool found_usable_info = false;

  if (symbol_data != ONLY_CFI) {
#ifndef NO_STABS_SUPPORT
    
    const Shdr* stab_section =
      FindElfSectionByName<ElfClass>(".stab", SHT_PROGBITS,
                                     sections, names, names_end,
                                     elf_header->e_shnum);
    if (stab_section) {
      const Shdr* stabstr_section = stab_section->sh_link + sections;
      if (stabstr_section) {
        found_debug_info_section = true;
        found_usable_info = true;
        info->LoadedSection(".stab");
        if (!LoadStabs<ElfClass>(elf_header, stab_section, stabstr_section,
                                 big_endian, module)) {
          fprintf(stderr, "%s: \".stab\" section found, but failed to load"
                  " STABS debugging information\n", obj_file.c_str());
        }
      }
    }
#endif  

    
    const Shdr* dwarf_section =
      FindElfSectionByName<ElfClass>(".debug_info", SHT_PROGBITS,
                                     sections, names, names_end,
                                     elf_header->e_shnum);
    if (dwarf_section) {
      found_debug_info_section = true;
      found_usable_info = true;
      info->LoadedSection(".debug_info");
      if (!LoadDwarf<ElfClass>(obj_file, elf_header, big_endian, module))
        fprintf(stderr, "%s: \".debug_info\" section found, but failed to load "
                "DWARF debugging information\n", obj_file.c_str());
    }
  }

  if (symbol_data != NO_CFI) {
    
    
    const Shdr* dwarf_cfi_section =
        FindElfSectionByName<ElfClass>(".debug_frame", SHT_PROGBITS,
                                       sections, names, names_end,
                                       elf_header->e_shnum);
    if (dwarf_cfi_section) {
      
      
      
      info->LoadedSection(".debug_frame");
      bool result =
          LoadDwarfCFI<ElfClass>(obj_file, elf_header, ".debug_frame",
                                 dwarf_cfi_section, false, 0, 0, big_endian,
                                 module);
      found_usable_info = found_usable_info || result;
      if (result)
        BPLOG(INFO) << "LoadSymbols:   read CFI from .debug_frame";
    }

    
    
    const Shdr* eh_frame_section =
        FindElfSectionByName<ElfClass>(".eh_frame", SHT_PROGBITS,
                                       sections, names, names_end,
                                       elf_header->e_shnum);
    if (eh_frame_section) {
      
      
      const Shdr* got_section =
          FindElfSectionByName<ElfClass>(".got", SHT_PROGBITS,
                                         sections, names, names_end,
                                         elf_header->e_shnum);
      const Shdr* text_section =
          FindElfSectionByName<ElfClass>(".text", SHT_PROGBITS,
                                         sections, names, names_end,
                                         elf_header->e_shnum);
      info->LoadedSection(".eh_frame");
      
      bool result =
          LoadDwarfCFI<ElfClass>(obj_file, elf_header, ".eh_frame",
                                 eh_frame_section, true,
                                 got_section, text_section, big_endian, module);
      found_usable_info = found_usable_info || result;
      if (result)
        BPLOG(INFO) << "LoadSymbols:   read CFI from .eh_frame";
    }
  }

  
  const Shdr* arm_exidx_section =
      FindElfSectionByName<ElfClass>(".ARM.exidx", SHT_ARM_EXIDX,
                                     sections, names, names_end,
                                     elf_header->e_shnum);
  const Shdr* arm_extab_section =
      FindElfSectionByName<ElfClass>(".ARM.extab", SHT_PROGBITS,
                                     sections, names, names_end,
                                     elf_header->e_shnum);
  
  
  
  
  
  if (arm_exidx_section && arm_extab_section && symbol_data != NO_CFI) {
    info->LoadedSection(".ARM.exidx");
    info->LoadedSection(".ARM.extab");
    bool result = LoadARMexidx<ElfClass>(elf_header,
                                         arm_exidx_section, arm_extab_section,
                                         loading_addr, module);
    found_usable_info = found_usable_info || result;
    if (result)
      BPLOG(INFO) << "LoadSymbols:   read EXIDX from .ARM.{exidx,extab}";
  }

  if (!found_debug_info_section && symbol_data != ONLY_CFI) {
    fprintf(stderr, "%s: file contains no debugging information"
            " (no \".stab\" or \".debug_info\" sections)\n",
            obj_file.c_str());

    
    if (read_gnu_debug_link) {
      const Shdr* gnu_debuglink_section
          = FindElfSectionByName<ElfClass>(".gnu_debuglink", SHT_PROGBITS,
                                           sections, names,
                                           names_end, elf_header->e_shnum);
      if (gnu_debuglink_section) {
        if (!info->debug_dirs().empty()) {
          found_debug_info_section = true;

          const char* debuglink_contents =
              GetOffset<ElfClass, char>(elf_header,
                                        gnu_debuglink_section->sh_offset);
          string debuglink_file
              = ReadDebugLink<ElfClass>(debuglink_contents,
                                        gnu_debuglink_section->sh_size,
                                        obj_file, info->debug_dirs());
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
  }

  if (symbol_data != ONLY_CFI) {
    const Shdr* dynsym_section =
      FindElfSectionByName<ElfClass>(".dynsym", SHT_DYNSYM,
                                     sections, names, names_end,
                                     elf_header->e_shnum);
    const Shdr* dynstr_section =
      FindElfSectionByName<ElfClass>(".dynstr", SHT_STRTAB,
                                     sections, names, names_end,
                                     elf_header->e_shnum);
    if (dynsym_section && dynstr_section) {
      info->LoadedSection(".dynsym");

      const uint8_t* dynsyms =
          GetOffset<ElfClass, uint8_t>(elf_header,
                                       dynsym_section->sh_offset);
      const uint8_t* dynstrs =
          GetOffset<ElfClass, uint8_t>(elf_header,
                                       dynstr_section->sh_offset);
      bool result =
          ELFSymbolsToModule(dynsyms,
                             dynsym_section->sh_size,
                             dynstrs,
                             dynstr_section->sh_size,
                             big_endian,
                             ElfClass::kAddrSize,
                             module);
      found_usable_info = found_usable_info || result;
    }
  }

  if (read_gnu_debug_link) {
    return found_debug_info_section;
  }

  
  BPLOG(INFO) << "LoadSymbols: "
              << (found_usable_info ? "SUCCESS " : "FAILURE ")
              << obj_file;
  return found_usable_info;
}



template<typename ElfClass>
const char* ElfArchitecture(const typename ElfClass::Ehdr* elf_header) {
  typedef typename ElfClass::Half Half;
  Half arch = elf_header->e_machine;
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



string FormatIdentifier(unsigned char identifier[16]) {
  char identifier_str[40];
  google_breakpad::FileID::ConvertIdentifierToString(
      identifier,
      identifier_str,
      sizeof(identifier_str));
  string id_no_dash;
  for (int i = 0; identifier_str[i] != '\0'; ++i)
    if (identifier_str[i] != '-')
      id_no_dash += identifier_str[i];
  
  
  
  id_no_dash += '0';
  return id_no_dash;
}



string BaseFileName(const string &filename) {
  
  char *c_filename = strdup(filename.c_str());
  string base = basename(c_filename);
  free(c_filename);
  return base;
}

template<typename ElfClass>
bool ReadSymbolDataElfClass(const typename ElfClass::Ehdr* elf_header,
                             const string& obj_filename,
                             const std::vector<string>& debug_dirs,
                             SymbolData symbol_data,
                             Module** out_module) {
  typedef typename ElfClass::Ehdr Ehdr;
  typedef typename ElfClass::Shdr Shdr;

  *out_module = NULL;

  unsigned char identifier[16];
  if (!google_breakpad::FileID::ElfFileIdentifierFromMappedFile(elf_header,
                                                                identifier)) {
    fprintf(stderr, "%s: unable to generate file identifier\n",
            obj_filename.c_str());
    return false;
  }

  const char *architecture = ElfArchitecture<ElfClass>(elf_header);
  if (!architecture) {
    fprintf(stderr, "%s: unrecognized ELF machine architecture: %d\n",
            obj_filename.c_str(), elf_header->e_machine);
    return false;
  }

  
  bool big_endian;
  if (!ElfEndianness<ElfClass>(elf_header, &big_endian))
    return false;

  string name = BaseFileName(obj_filename);
  string os = "Linux";
  string id = FormatIdentifier(identifier);

  LoadSymbolsInfo<ElfClass> info(debug_dirs);
  scoped_ptr<Module> module(new Module(name, os, architecture, id));
  if (!LoadSymbols<ElfClass>(obj_filename, big_endian, elf_header,
                             !debug_dirs.empty(), &info,
                             symbol_data, module.get())) {
    const string debuglink_file = info.debuglink_file();
    if (debuglink_file.empty())
      return false;

    
    fprintf(stderr, "Found debugging info in %s\n", debuglink_file.c_str());
    MmapWrapper debug_map_wrapper;
    Ehdr* debug_elf_header = NULL;
    if (!LoadELF(debuglink_file, &debug_map_wrapper,
                 reinterpret_cast<void**>(&debug_elf_header)))
      return false;
    
    const char *debug_architecture =
        ElfArchitecture<ElfClass>(debug_elf_header);
    if (!debug_architecture) {
      fprintf(stderr, "%s: unrecognized ELF machine architecture: %d\n",
              debuglink_file.c_str(), debug_elf_header->e_machine);
      return false;
    }
    if (strcmp(architecture, debug_architecture)) {
      fprintf(stderr, "%s with ELF machine architecture %s does not match "
              "%s with ELF architecture %s\n",
              debuglink_file.c_str(), debug_architecture,
              obj_filename.c_str(), architecture);
      return false;
    }

    bool debug_big_endian;
    if (!ElfEndianness<ElfClass>(debug_elf_header, &debug_big_endian))
      return false;
    if (debug_big_endian != big_endian) {
      fprintf(stderr, "%s and %s does not match in endianness\n",
              obj_filename.c_str(), debuglink_file.c_str());
      return false;
    }

    if (!LoadSymbols<ElfClass>(debuglink_file, debug_big_endian,
                               debug_elf_header, false, &info,
                               symbol_data, module.get())) {
      return false;
    }
  }

  *out_module = module.release();
  return true;
}

}  

namespace google_breakpad {


bool ReadSymbolDataInternal(const uint8_t* obj_file,
                            const string& obj_filename,
                            const std::vector<string>& debug_dirs,
                            SymbolData symbol_data,
                            Module** module) {

  if (!IsValidElf(obj_file)) {
    fprintf(stderr, "Not a valid ELF file: %s\n", obj_filename.c_str());
    return false;
  }

  int elfclass = ElfClass(obj_file);
  if (elfclass == ELFCLASS32) {
    return ReadSymbolDataElfClass<ElfClass32>(
        reinterpret_cast<const Elf32_Ehdr*>(obj_file), obj_filename, debug_dirs,
        symbol_data, module);
  }
  if (elfclass == ELFCLASS64) {
    return ReadSymbolDataElfClass<ElfClass64>(
        reinterpret_cast<const Elf64_Ehdr*>(obj_file), obj_filename, debug_dirs,
        symbol_data, module);
  }

  return false;
}

bool WriteSymbolFile(const string &obj_file,
                     const std::vector<string>& debug_dirs,
                     SymbolData symbol_data,
                     std::ostream &sym_stream) {
  Module* module;
  if (!ReadSymbolData(obj_file, debug_dirs, symbol_data, &module))
    return false;

  bool result = module->Write(sym_stream, symbol_data);
  delete module;
  return result;
}

bool ReadSymbolData(const string& obj_file,
                    const std::vector<string>& debug_dirs,
                    SymbolData symbol_data,
                    Module** module) {
  MmapWrapper map_wrapper;
  void* elf_header = NULL;
  if (!LoadELF(obj_file, &map_wrapper, &elf_header))
    return false;

  return ReadSymbolDataInternal(reinterpret_cast<uint8_t*>(elf_header),
                                obj_file, debug_dirs, symbol_data, module);
}

}  
