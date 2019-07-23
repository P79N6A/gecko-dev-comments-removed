






























#include <demangle.h>
#include <fcntl.h>
#include <gelf.h>
#include <link.h>
#include <sys/mman.h>
#include <stab.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <functional>
#include <vector>

#include "common/solaris/dump_symbols.h"
#include "common/solaris/file_id.h"
#include "common/solaris/guid_creator.h"
#include "processor/scoped_ptr.h"


namespace {


struct slist {
  
  unsigned int n_strx;
  
  unsigned char n_type;
  char n_other;
  short n_desc;
  unsigned long n_value;
};


struct LineInfo {
  
  
  GElf_Off rva_to_func;
  
  GElf_Off rva_to_base;
  
  
  
  
  uint32_t size;
  
  uint32_t line_num;
};


struct FuncInfo {
  
  const char *name;
  
  GElf_Off rva_to_base;
  
  
  GElf_Addr addr;
  
  
  uint32_t size;
  
  uint32_t stack_param_size;
  
  std::vector<struct LineInfo> line_info;
};


struct SourceFileInfo {
  
  const char *name;
  
  GElf_Addr addr;
  
  int source_id;
  
  std::vector<struct FuncInfo> func_info;
};



struct SymbolInfo {
  std::vector<struct SourceFileInfo> source_file_info;
};


const char *kStabName = ".stab";


const char *kStabStrName = ".stabstr";


const int demangleLen = 2000;


std::string Demangle(const char *mangled) {
  int status = 0;
  char *demangled = (char *)malloc(demangleLen);
  if (!demangled) {
    fprintf(stderr, "no enough memory.\n");
    goto out;
  }

  if ((status = cplus_demangle(mangled, demangled, demangleLen)) ==
      DEMANGLE_ESPACE) {
    fprintf(stderr, "incorrect demangle.\n");
    goto out;
  }

  std::string str(demangled);
  free(demangled);
  return str;

out:
  return std::string(mangled);
}


GElf_Addr GetLoadingAddress(const GElf_Phdr *program_headers, int nheader) {
  for (int i = 0; i < nheader; ++i) {
    const GElf_Phdr &header = program_headers[i];
    
    if (header.p_type == PT_LOAD && header.p_offset == 0)
      return header.p_vaddr;
  }
  
  return 0;
}

bool WriteFormat(int fd, const char *fmt, ...) {
  va_list list;
  char buffer[4096];
  ssize_t expected, written;
  va_start(list, fmt);
  vsnprintf(buffer, sizeof(buffer), fmt, list);
  expected = strlen(buffer);
  written = write(fd, buffer, strlen(buffer));
  va_end(list);
  return expected == written;
}

bool IsValidElf(const GElf_Ehdr *elf_header) {
  return memcmp(elf_header, ELFMAG, SELFMAG) == 0;
}

static bool FindSectionByName(Elf *elf, const char *name,
                              int shstrndx,
                              GElf_Shdr *shdr) {
  assert(name != NULL);

  if (strlen(name) == 0)
    return false;

  Elf_Scn *scn = NULL;

  while ((scn = elf_nextscn(elf, scn)) != NULL) {
    if (gelf_getshdr(scn, shdr) == (GElf_Shdr *)0) {
      fprintf(stderr, "failed to read section header: %s\n", elf_errmsg(0));
      return false;
    }

    const char *section_name = elf_strptr(elf, shstrndx, shdr->sh_name);
    if (!section_name) {
      fprintf(stderr, "Section name error: %s\n", elf_errmsg(-1));
      continue;
    }

    if (strcmp(section_name, name) == 0)
      return true;
  }

  return false;
}




int LoadStackParamSize(struct slist *list,
                       struct slist *list_end,
                       struct FuncInfo *func_info) {
  struct slist *cur_list = list;
  int step = 1;
  while (cur_list < list_end && cur_list->n_type == N_PSYM) {
    ++cur_list;
    ++step;
  }

  func_info->stack_param_size = 0;
  return step;
}

int LoadLineInfo(struct slist *list,
                 struct slist *list_end,
                 struct FuncInfo *func_info) {
  struct slist *cur_list = list;
  do {
    
    while (cur_list < list_end && cur_list->n_type != N_SLINE) {
      
      if (cur_list->n_type == N_FUN || cur_list->n_type == N_SO)
        return cur_list - list;
      ++cur_list;
    }
    struct LineInfo line;
    while (cur_list < list_end && cur_list->n_type == N_SLINE) {
      line.rva_to_func = cur_list->n_value;
      
      line.line_num = (unsigned short)cur_list->n_desc;
      func_info->line_info.push_back(line);
      ++cur_list;
    }
    if (cur_list == list_end && cur_list->n_type == N_ENDM)
      break;
  } while (list < list_end);

  return cur_list - list;
}

int LoadFuncSymbols(struct slist *list,
                    struct slist *list_end,
                    const GElf_Shdr *stabstr_section,
                    GElf_Word base,
                    struct SourceFileInfo *source_file_info) {
  struct slist *cur_list = list;
  assert(cur_list->n_type == N_SO);
  ++cur_list;

  source_file_info->func_info.clear();
  while (cur_list < list_end) {
    
    while (cur_list < list_end && cur_list->n_type != N_FUN) {
      if (cur_list->n_type == N_SO) {
        return cur_list - list;
      }
      ++cur_list;
      continue;
    }
    while (cur_list->n_type == N_FUN) {
      struct FuncInfo func_info;
      memset(&func_info, 0, sizeof(func_info));
      func_info.name =
        reinterpret_cast<char *>(cur_list->n_strx +
                                 stabstr_section->sh_offset + base);
      
      
      func_info.addr = cur_list->n_value;
      ++cur_list;
      if (cur_list->n_type != N_ESYM && cur_list->n_type != N_ISYM &&
          cur_list->n_type != N_FUN) {
        
        cur_list += LoadStackParamSize(cur_list, list_end, &func_info);
        
        cur_list += LoadLineInfo(cur_list, list_end, &func_info);
      }
      
      
      
      
      
      if (func_info.addr >= source_file_info->addr) {
        source_file_info->func_info.push_back(func_info);
      }
    }
  }
  return cur_list - list;
}


bool ComputeSizeAndRVA(GElf_Addr loading_addr, struct SymbolInfo *symbols) {
  std::vector<struct SourceFileInfo> *sorted_files =
    &(symbols->source_file_info);
  for (size_t i = 0; i < sorted_files->size(); ++i) {
    struct SourceFileInfo &source_file = (*sorted_files)[i];
    std::vector<struct FuncInfo> *sorted_functions = &(source_file.func_info);
    for (size_t j = 0; j < sorted_functions->size(); ++j) {
      struct FuncInfo &func_info = (*sorted_functions)[j];
      assert(func_info.addr >= loading_addr);
      func_info.rva_to_base = func_info.addr - loading_addr;
      int line_count = func_info.line_info.size();
      func_info.size =
        (line_count == 0) ? 0 :
                            func_info.line_info[line_count - 1].rva_to_func;
      
      for (size_t k = 0; k < line_count; ++k) {
        struct LineInfo &line_info = func_info.line_info[k];
        if (k == 0) {
          line_info.size = line_info.rva_to_func;
        } else {
          line_info.size =
            line_info.rva_to_func - func_info.line_info[k - 1].rva_to_func;
        }
        line_info.rva_to_base = line_info.rva_to_func + func_info.rva_to_base;
      }  
    }  
  }  
  return true;
}

bool LoadAllSymbols(const GElf_Shdr *stab_section,
                    const GElf_Shdr *stabstr_section,
                    GElf_Addr loading_addr,
                    GElf_Word base,
                    struct SymbolInfo *symbols) {
  if (stab_section == NULL || stabstr_section == NULL)
    return false;

  struct slist *lists =
    reinterpret_cast<struct slist *>(stab_section->sh_offset + base);
  int nstab = stab_section->sh_size / sizeof(struct slist);
  int source_id = 0;
  
  for (int i = 0; i < nstab; ) {
    int step = 1;
    struct slist *cur_list = lists + i;
    if (cur_list->n_type == N_SO) {
      
      struct SourceFileInfo source_file_info;
      source_file_info.name =
        reinterpret_cast<char *>(cur_list->n_strx +
                                 stabstr_section->sh_offset + base);
      
      
      source_file_info.addr = cur_list->n_value;
      if (strchr(source_file_info.name, '.'))
        source_file_info.source_id = source_id++;
      else
        source_file_info.source_id = -1;
      step = LoadFuncSymbols(cur_list, lists + nstab - 1,
                             stabstr_section, base, &source_file_info);
      symbols->source_file_info.push_back(source_file_info);
    }
    i += step;
  }
  
  return ComputeSizeAndRVA(loading_addr, symbols);
}

bool LoadSymbols(Elf *elf, GElf_Ehdr *elf_header, struct SymbolInfo *symbols,
                 void *obj_base) {
  GElf_Word base = reinterpret_cast<GElf_Word>(obj_base);
  GElf_Addr loading_addr = GetLoadingAddress(
      reinterpret_cast<GElf_Phdr *>(elf_header->e_phoff + base),
      elf_header->e_phnum);

  const GElf_Shdr *sections =
    reinterpret_cast<GElf_Shdr *>(elf_header->e_shoff + base);
  GElf_Shdr stab_section;
  if (!FindSectionByName(elf, kStabName, elf_header->e_shstrndx,
                         &stab_section)) {
    fprintf(stderr, "Stab section not found.\n");
    return false;
  }
  GElf_Shdr stabstr_section;
  if (!FindSectionByName(elf, kStabStrName, elf_header->e_shstrndx,
                         &stabstr_section)) {
    fprintf(stderr, "Stabstr section not found.\n");
    return false;
  }

  
  return LoadAllSymbols(&stab_section, &stabstr_section, loading_addr, base, symbols);
}

bool WriteModuleInfo(int fd, GElf_Half arch, const std::string &obj_file) {
  const char *arch_name = NULL;
  if (arch == EM_386)
    arch_name = "x86";
  else if (arch == EM_X86_64)
    arch_name = "x86_64";
  else
    return false;

  unsigned char identifier[16];
  google_breakpad::FileID file_id(obj_file.c_str());
  if (file_id.ElfFileIdentifier(identifier)) {
    char identifier_str[40];
    file_id.ConvertIdentifierToString(identifier,
                                      identifier_str, sizeof(identifier_str));
    std::string filename = obj_file;
    size_t slash_pos = obj_file.find_last_of("/");
    if (slash_pos != std::string::npos)
      filename = obj_file.substr(slash_pos + 1);
    return WriteFormat(fd, "MODULE solaris %s %s %s\n", arch_name,
                       identifier_str, filename.c_str());
  }
  return false;
}

bool WriteSourceFileInfo(int fd, const struct SymbolInfo &symbols) {
  for (size_t i = 0; i < symbols.source_file_info.size(); ++i) {
    if (symbols.source_file_info[i].source_id != -1) {
      const char *name = symbols.source_file_info[i].name;
      if (!WriteFormat(fd, "FILE %d %s\n",
                       symbols.source_file_info[i].source_id, name))
        return false;
    }
  }
  return true;
}

bool WriteOneFunction(int fd, int source_id,
                      const struct FuncInfo &func_info){
  
  std::string func_name(func_info.name);
  std::string::size_type last_colon = func_name.find_last_of(':');
  if (last_colon != std::string::npos)
    func_name = func_name.substr(0, last_colon);
  func_name = Demangle(func_name.c_str());

  if (func_info.size < 0)
    return true;

  
  if (WriteFormat(fd, "FUNC %llx %d %d %s\n",
                  (long long)func_info.rva_to_base,
                  func_info.size,
                  func_info.stack_param_size,
                  func_name.c_str())) {
    for (size_t i = 0; i < func_info.line_info.size(); ++i) {
      const struct LineInfo &line_info = func_info.line_info[i];
      if (!WriteFormat(fd, "%llx %d %d %d\n",
                       (long long)line_info.rva_to_base,
                       line_info.size,
                       line_info.line_num,
                       source_id))
        return false;
    }
    return true;
  }
  return false;
}

bool WriteFunctionInfo(int fd, const struct SymbolInfo &symbols) {
  for (size_t i = 0; i < symbols.source_file_info.size(); ++i) {
    const struct SourceFileInfo &file_info = symbols.source_file_info[i];
    for (size_t j = 0; j < file_info.func_info.size(); ++j) {
      const struct FuncInfo &func_info = file_info.func_info[j];
      if (!WriteOneFunction(fd, file_info.source_id, func_info))
        return false;
    }
  }
  return true;
}

bool DumpStabSymbols(int fd, const struct SymbolInfo &symbols) {
  return WriteSourceFileInfo(fd, symbols) &&
    WriteFunctionInfo(fd, symbols);
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
      munmap((char *)base_, size_);
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

}  

namespace google_breakpad {

class AutoElfEnder {
 public:
  AutoElfEnder(Elf *elf) : elf_(elf) {}
  ~AutoElfEnder() { if (elf_) elf_end(elf_); }
 private:
  Elf *elf_;
};


bool DumpSymbols::WriteSymbolFile(const std::string &obj_file, int sym_fd) {
  if (elf_version(EV_CURRENT) == EV_NONE) {
    fprintf(stderr, "elf_version() failed: %s\n", elf_errmsg(0));
    return false;
  }

  int obj_fd = open(obj_file.c_str(), O_RDONLY);
  if (obj_fd < 0)
    return false;
  FDWrapper obj_fd_wrapper(obj_fd);
  struct stat st;
  if (fstat(obj_fd, &st) != 0 && st.st_size <= 0)
    return false;
  void *obj_base = mmap(NULL, st.st_size,
                        PROT_READ, MAP_PRIVATE, obj_fd, 0);
  if (!obj_base)
    return false;
  MmapWrapper map_wrapper(obj_base, st.st_size);
  GElf_Ehdr elf_header;
  Elf *elf = elf_begin(obj_fd, ELF_C_READ, NULL);
  AutoElfEnder elfEnder(elf);

  if (gelf_getehdr(elf, &elf_header) == (GElf_Ehdr *)NULL) {
    fprintf(stderr, "failed to read elf header: %s\n", elf_errmsg(-1));
    return false;
  }

  if (!IsValidElf(&elf_header)) {
    fprintf(stderr, "header magic doesn't match\n");
    return false;
  }
  struct SymbolInfo symbols;
  if (!LoadSymbols(elf, &elf_header, &symbols, obj_base))
    return false;
  
  if (WriteModuleInfo(sym_fd, elf_header.e_machine, obj_file) &&
      DumpStabSymbols(sym_fd, symbols))
    return true;

  return false;
}

}  
