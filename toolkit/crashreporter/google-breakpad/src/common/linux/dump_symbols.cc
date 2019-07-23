




























#include <assert.h>
#include <cxxabi.h>
#include <elf.h>
#include <errno.h>
#include <fcntl.h>
#include <link.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <algorithm>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <list>
#include <map>
#include <string>
#include <vector>

#include "common/linux/dump_symbols.h"
#include "common/linux/file_id.h"
#include "common/linux/module.h"
#include "common/linux/stabs_reader.h"


namespace {

using google_breakpad::Module;
using std::vector;


static const char *kStabName = ".stab";



static std::string Demangle(const std::string &mangled) {
  int status = 0;
  char *demangled = abi::__cxa_demangle(mangled.c_str(), NULL, NULL, &status);
  if (status == 0 && demangled != NULL) {
    std::string str(demangled);
    free(demangled);
    return str;
  }
  return std::string(mangled);
}



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
                                           const ElfW(Shdr) *strtab,
                                           int nsection) {
  assert(name != NULL);
  assert(sections != NULL);
  assert(nsection > 0);

  int name_len = strlen(name);
  if (name_len == 0)
    return NULL;

  for (int i = 0; i < nsection; ++i) {
    const char *section_name =
      reinterpret_cast<char*>(strtab->sh_offset + sections[i].sh_name);
    if (!strncmp(name, section_name, name_len))
      return sections + i;
  }
  return NULL;
}


class DumpStabsHandler: public google_breakpad::StabsHandler {
 public:
  DumpStabsHandler(Module *module) :
      module_(module),
      comp_unit_base_address_(0),
      current_function_(NULL),
      current_source_file_(NULL),
      current_source_file_name_(NULL) { }

  bool StartCompilationUnit(const char *name, uint64_t address,
                            const char *build_directory);
  bool EndCompilationUnit(uint64_t address);
  bool StartFunction(const std::string &name, uint64_t address);
  bool EndFunction(uint64_t address);
  bool Line(uint64_t address, const char *name, int number);

  
  
  
  
  
  
  
  void Finalize();

 private:

  
  
  static const uint64_t kFallbackSize = 0x10000000;

  
  Module *module_;

  
  
  
  
  
  
  
  vector<Module::Function *> functions_;

  
  
  
  vector<Module::Address> boundaries_;

  
  
  
  
  Module::Address comp_unit_base_address_;

  
  Module::Function *current_function_;

  
  Module::File *current_source_file_;

  
  
  
  
  const char *current_source_file_name_;
};
    
bool DumpStabsHandler::StartCompilationUnit(const char *name, uint64_t address,
                                            const char *build_directory) {
  assert(! comp_unit_base_address_);
  current_source_file_name_ = name;
  current_source_file_ = module_->FindFile(name);
  comp_unit_base_address_ = address;
  boundaries_.push_back(static_cast<Module::Address>(address));
  return true;
}

bool DumpStabsHandler::EndCompilationUnit(uint64_t address) {
  assert(comp_unit_base_address_);
  comp_unit_base_address_ = 0;
  current_source_file_ = NULL;
  current_source_file_name_ = NULL;
  if (address)
    boundaries_.push_back(static_cast<Module::Address>(address));
  return true;
}

bool DumpStabsHandler::StartFunction(const std::string &name,
                                     uint64_t address) {
  assert(! current_function_);
  Module::Function *f = new Module::Function;
  f->name_ = Demangle(name);
  f->address_ = address;
  f->size_ = 0;           
  f->parameter_size_ = 0; 
  current_function_ = f;
  boundaries_.push_back(static_cast<Module::Address>(address));
  return true;
}

bool DumpStabsHandler::EndFunction(uint64_t address) {
  assert(current_function_);
  
  
  
  
  
  
  
  
  if (current_function_->address_ >= comp_unit_base_address_)
    functions_.push_back(current_function_);
  else
    delete current_function_;
  current_function_ = NULL;
  if (address)
    boundaries_.push_back(static_cast<Module::Address>(address));
  return true;
}

bool DumpStabsHandler::Line(uint64_t address, const char *name, int number) {
  assert(current_function_);
  assert(current_source_file_);
  if (name != current_source_file_name_) {
    current_source_file_ = module_->FindFile(name);
    current_source_file_name_ = name;
  }
  Module::Line line;
  line.address_ = address;
  line.size_ = 0;  
  line.file_ = current_source_file_;
  line.number_ = number;
  current_function_->lines_.push_back(line);
  return true;
}

void DumpStabsHandler::Finalize() {
  
  sort(boundaries_.begin(), boundaries_.end());
  
  sort(functions_.begin(), functions_.end(),
       Module::Function::CompareByAddress);
  for (vector<Module::Function *>::iterator func_it = functions_.begin();
       func_it != functions_.end();
       func_it++) {
    Module::Function *f = *func_it;
    
    vector<Module::Address>::iterator boundary
        = std::upper_bound(boundaries_.begin(), boundaries_.end(), f->address_);
    if (boundary != boundaries_.end())
      f->size_ = *boundary - f->address_;
    else
      
      
      
      
      
      f->size_ = kFallbackSize;

    
    if (! f->lines_.empty()) {
      stable_sort(f->lines_.begin(), f->lines_.end(),
                  Module::Line::CompareByAddress);
      vector<Module::Line>::iterator last_line = f->lines_.end() - 1;
      for (vector<Module::Line>::iterator line_it = f->lines_.begin();
           line_it != last_line; line_it++)
        line_it[0].size_ = line_it[1].address_ - line_it[0].address_;
      
      last_line->size_ = (f->address_ + f->size_) - last_line->address_;
    }
  }
  
  
  module_->AddFunctions(functions_.begin(), functions_.end());
  functions_.clear();
}

static bool LoadSymbols(const ElfW(Shdr) *stab_section,
                        const ElfW(Shdr) *stabstr_section,
                        Module *module) {
  if (stab_section == NULL || stabstr_section == NULL)
    return false;

  
  DumpStabsHandler handler(module);
  
  uint8_t *stabs = reinterpret_cast<uint8_t *>(stab_section->sh_offset);
  uint8_t *stabstr = reinterpret_cast<uint8_t *>(stabstr_section->sh_offset);
  google_breakpad::StabsReader reader(stabs, stab_section->sh_size,
                                      stabstr, stabstr_section->sh_size,
                                      &handler);
  
  if (! reader.Process())
    return false;
  handler.Finalize();
  return true;
}

static bool LoadSymbols(ElfW(Ehdr) *elf_header, Module *module) {
  
  FixAddress(elf_header);
  ElfW(Addr) loading_addr = GetLoadingAddress(
      reinterpret_cast<ElfW(Phdr) *>(elf_header->e_phoff),
      elf_header->e_phnum);
  module->SetLoadAddress(loading_addr);

  const ElfW(Shdr) *sections =
    reinterpret_cast<ElfW(Shdr) *>(elf_header->e_shoff);
  const ElfW(Shdr) *strtab = sections + elf_header->e_shstrndx;
  const ElfW(Shdr) *stab_section =
    FindSectionByName(kStabName, sections, strtab, elf_header->e_shnum);
  if (stab_section == NULL) {
    fprintf(stderr, "Stab section not found.\n");
    return false;
  }
  const ElfW(Shdr) *stabstr_section = stab_section->sh_link + sections;

  
  return LoadSymbols(stab_section, stabstr_section, module);
}






class FDWrapper {
 public:
  explicit FDWrapper(int fd) :
    fd_(fd) {
    }
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
   MmapWrapper(void *mapped_address, size_t mapped_size) :
     base_(mapped_address), size_(mapped_size) {
   }
   ~MmapWrapper() {
     if (base_ != NULL) {
       assert(size_ > 0);
       munmap(base_, size_);
     }
   }
   void release() {
     base_ = NULL;
     size_ = 0;
   }

  private:
   void *base_;
   size_t size_;
};



const char *ElfArchitecture(const ElfW(Ehdr) *elf_header) {
  ElfW(Half) arch = elf_header->e_machine;
  if (arch == EM_386)
    return "x86";
  else if (arch == EM_X86_64)
    return "x86_64";
  else
    return NULL;
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

bool DumpSymbols::WriteSymbolFile(const std::string &obj_file,
                                  FILE *sym_file) {
  int obj_fd = open(obj_file.c_str(), O_RDONLY);
  if (obj_fd < 0)
    return false;
  FDWrapper obj_fd_wrapper(obj_fd);
  struct stat st;
  if (fstat(obj_fd, &st) != 0 && st.st_size <= 0)
    return false;
  void *obj_base = mmap(NULL, st.st_size,
                        PROT_READ | PROT_WRITE, MAP_PRIVATE, obj_fd, 0);
  if (obj_base == MAP_FAILED)
    return false;
  MmapWrapper map_wrapper(obj_base, st.st_size);
  ElfW(Ehdr) *elf_header = reinterpret_cast<ElfW(Ehdr) *>(obj_base);
  if (!IsValidElf(elf_header))
    return false;

  unsigned char identifier[16];
  google_breakpad::FileID file_id(obj_file.c_str());
  if (! file_id.ElfFileIdentifier(identifier))
    return false;

  const char *architecture = ElfArchitecture(elf_header);
  if (! architecture)
    return false;

  std::string name = BaseFileName(obj_file);
  std::string os = "Linux";
  std::string id = FormatIdentifier(identifier);

  Module module(name, os, architecture, id);
  if (!LoadSymbols(elf_header, &module))
    return false;
  if (!module.Write(sym_file))
    return false;

  return true;
}

}  
