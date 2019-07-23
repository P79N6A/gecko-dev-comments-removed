




























#include <a.out.h>
#include <cstdarg>
#include <cstdlib>
#include <cstdio>
#include <cxxabi.h>
#include <elf.h>
#include <errno.h>
#include <fcntl.h>
#include <link.h>
#include <sys/mman.h>
#include <stab.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <algorithm>

#include <functional>
#include <list>
#include <vector>
#include <string.h>

#include "common/linux/dump_symbols.h"
#include "common/linux/file_id.h"
#include "common/linux/guid_creator.h"
#include "processor/scoped_ptr.h"


namespace {


struct LineInfo {
  
  
  
  uint32_t source_name_index;
  
  
  ElfW(Off) rva_to_func;
  
  ElfW(Off) rva_to_base;
  
  
  
  uint32_t size;
  
  uint32_t line_num;
  
  int source_id;
};

typedef std::list<struct LineInfo> LineInfoList;


struct FuncInfo {
  
  const char *name;
  
  ElfW(Off) rva_to_base;
  
  
  ElfW(Addr) addr;
  
  
  
  uint32_t size;
  
  uint32_t stack_param_size;
  
  bool has_sol;
  
  LineInfoList line_info;
};

typedef std::list<struct FuncInfo> FuncInfoList;


struct SourceFileInfo {
  
  uint32_t name_index;
  
  const char *name;
  
  ElfW(Addr) addr;
  
  int source_id;
  
  FuncInfoList func_info;
};

typedef std::list<struct SourceFileInfo> SourceFileInfoList;



struct SymbolInfo {
  SourceFileInfoList source_file_info;

  
  int next_source_id;
};


static const char *kStabName = ".stab";



static std::string Demangle(const char *mangled) {
  int status = 0;
  char *demangled = abi::__cxa_demangle(mangled, NULL, NULL, &status);
  if (status == 0 && demangled != NULL) {
    std::string str(demangled);
    free(demangled);
    return str;
  }
  return std::string(mangled);
}



static void FixAddress(void *obj_base) {
  ElfW(Word) base = reinterpret_cast<ElfW(Word)>(obj_base);
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
      (char*)(strtab->sh_offset + sections[i].sh_name);
    if (!strncmp(name, section_name, name_len))
      return sections + i;
  }
  return NULL;
}




static int LoadStackParamSize(struct nlist *list,
                              struct nlist *list_end,
                              struct FuncInfo *func_info) {
  struct nlist *cur_list = list;
  assert(cur_list->n_type == N_FUN);
  ++cur_list;
  int step = 1;
  while (cur_list < list_end && cur_list->n_type == N_PSYM) {
    ++cur_list;
    ++step;
  }
  func_info->stack_param_size = 0;
  return step;
}

static int LoadLineInfo(struct nlist *list,
                        struct nlist *list_end,
                        const struct SourceFileInfo &source_file_info,
                        struct FuncInfo *func_info) {
  struct nlist *cur_list = list;
  func_info->has_sol = false;
  
  
  
  
  int current_source_name_index = source_file_info.name_index;
  do {
    
    while (cur_list < list_end && cur_list->n_type != N_SLINE) {
      
      if (cur_list->n_type == N_FUN || cur_list->n_type == N_SO)
        return cur_list - list;
      
      
      if (cur_list->n_type == N_SOL) {
        func_info->has_sol = true;

        if (cur_list->n_un.n_strx > 0 &&
            cur_list->n_un.n_strx != current_source_name_index) {
          
          current_source_name_index = cur_list->n_un.n_strx;
        }
      }
      ++cur_list;
    }
    struct LineInfo line;
    while (cur_list < list_end && cur_list->n_type == N_SLINE) {
      line.source_name_index = current_source_name_index;
      line.rva_to_func = cur_list->n_value;
      
      line.line_num = (unsigned short)cur_list->n_desc;
      
      
      line.source_id = -1;
      func_info->line_info.push_back(line);
      ++cur_list;
    }
  } while (list < list_end);

  return cur_list - list;
}

static int LoadFuncSymbols(struct nlist *list,
                           struct nlist *list_end,
                           const ElfW(Shdr) *stabstr_section,
                           struct SourceFileInfo *source_file_info) {
  struct nlist *cur_list = list;
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
    if (cur_list->n_type == N_FUN) {
      struct FuncInfo func_info;
      func_info.name =
        reinterpret_cast<char *>(cur_list->n_un.n_strx +
                                 stabstr_section->sh_offset);
      func_info.addr = cur_list->n_value;
      func_info.rva_to_base = 0;
      func_info.size = 0;
      func_info.stack_param_size = 0;
      func_info.has_sol = 0;

      
      cur_list += LoadStackParamSize(cur_list, list_end, &func_info);
      
      cur_list += LoadLineInfo(cur_list,
                               list_end,
                               *source_file_info,
                               &func_info);

      
      
      
      
      if (func_info.addr >= source_file_info->addr) {
        source_file_info->func_info.push_back(func_info);
      }
    }
  }
  return cur_list - list;
}



template<class T1, class T2>
static bool CompareAddress(T1 *a, T2 *b) {
  return a->addr < b->addr;
}




template<class Container>
static std::vector<typename Container::value_type *> SortByAddress(
    Container *container) {
  typedef typename Container::iterator It;
  typedef typename Container::value_type T;
  std::vector<T *> sorted_array_ptr;
  sorted_array_ptr.reserve(container->size());
  for (It it = container->begin(); it != container->end(); it++)
    sorted_array_ptr.push_back(&(*it));
  std::sort(sorted_array_ptr.begin(),
            sorted_array_ptr.end(),
            std::ptr_fun(CompareAddress<T, T>));

  return sorted_array_ptr;
}



static ElfW(Addr) NextAddress(
    std::vector<struct FuncInfo *> *sorted_functions,
    std::vector<struct SourceFileInfo *> *sorted_files,
    const struct FuncInfo &func_info) {
  std::vector<struct FuncInfo *>::iterator next_func_iter =
    std::find_if(sorted_functions->begin(),
                 sorted_functions->end(),
                 std::bind1st(
                     std::ptr_fun(
                         CompareAddress<struct FuncInfo,
                                        struct FuncInfo>
                         ),
                     &func_info)
                );
  if (next_func_iter != sorted_functions->end())
    return (*next_func_iter)->addr;

  std::vector<struct SourceFileInfo *>::iterator next_file_iter =
    std::find_if(sorted_files->begin(),
                 sorted_files->end(),
                 std::bind1st(
                     std::ptr_fun(
                         CompareAddress<struct FuncInfo,
                                        struct SourceFileInfo>
                         ),
                     &func_info)
                );
  if (next_file_iter != sorted_files->end()) {
    return (*next_file_iter)->addr;
  }
  return 0;
}

static int FindFileByNameIdx(uint32_t name_index,
                             SourceFileInfoList &files) {
  for (SourceFileInfoList::iterator it = files.begin();
       it != files.end(); it++) {
    if (it->name_index == name_index)
      return it->source_id;
  }

  return -1;
}



static void AddIncludedFiles(struct SymbolInfo *symbols,
                             const ElfW(Shdr) *stabstr_section) {
  for (SourceFileInfoList::iterator source_file_it =
	 symbols->source_file_info.begin();
       source_file_it != symbols->source_file_info.end();
       ++source_file_it) {
    struct SourceFileInfo &source_file = *source_file_it;

    for (FuncInfoList::iterator func_info_it = source_file.func_info.begin(); 
	 func_info_it != source_file.func_info.end();
	 ++func_info_it) {
      struct FuncInfo &func_info = *func_info_it;

      for (LineInfoList::iterator line_info_it = func_info.line_info.begin(); 
	   line_info_it != func_info.line_info.end(); ++line_info_it) {
        struct LineInfo &line_info = *line_info_it;

        assert(line_info.source_name_index > 0);
        assert(source_file.name_index > 0);

        
        
        if (line_info.source_name_index != source_file.name_index) {
          
          
          int found_source_id = FindFileByNameIdx(line_info.source_name_index,
                                                  symbols->source_file_info);
          if (found_source_id < 0) {
            
            
            SourceFileInfo new_file;
            new_file.name_index = line_info.source_name_index;
            new_file.name = reinterpret_cast<char *>(new_file.name_index +
                                                     stabstr_section->sh_offset);
            new_file.addr = 0;
            new_file.source_id = symbols->next_source_id++;
            line_info.source_id = new_file.source_id;
            symbols->source_file_info.push_back(new_file);
          } else {
            
            line_info.source_id = found_source_id;
          }
        } else {
          
          line_info.source_id = source_file.source_id;
        }
      }  
    }  
  } 

}


static bool ComputeSizeAndRVA(ElfW(Addr) loading_addr,
                              struct SymbolInfo *symbols) {
  std::vector<struct SourceFileInfo *> sorted_files =
    SortByAddress(&(symbols->source_file_info));
  for (size_t i = 0; i < sorted_files.size(); ++i) {
    struct SourceFileInfo &source_file = *sorted_files[i];
    std::vector<struct FuncInfo *> sorted_functions =
      SortByAddress(&(source_file.func_info));
    for (size_t j = 0; j < sorted_functions.size(); ++j) {
      struct FuncInfo &func_info = *sorted_functions[j];
      assert(func_info.addr >= loading_addr);
      func_info.rva_to_base = func_info.addr - loading_addr;
      func_info.size = 0;
      ElfW(Addr) next_addr = NextAddress(&sorted_functions,
                                         &sorted_files,
                                         func_info);
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      static const int kDefaultSize = 0x10000000;
      static int no_next_addr_count = 0;
      if (next_addr != 0) {
        func_info.size = next_addr - func_info.addr;
      } else {
        if (no_next_addr_count > 1) {
          fprintf(stderr, "Got more than one funtion without the \
                  following symbol. Igore this function.\n");
          fprintf(stderr, "The dumped symbol may not correct.\n");
          assert(!"This should not happen!\n");
          func_info.size = 0;
          continue;
        }

        no_next_addr_count++;
        func_info.size = kDefaultSize;
      }
      
      for (LineInfoList::iterator line_info_it = func_info.line_info.begin(); 
	   line_info_it != func_info.line_info.end(); line_info_it++) {
        struct LineInfo &line_info = *line_info_it;
	LineInfoList::iterator next_line_info_it = line_info_it;
	next_line_info_it++;
        line_info.size = 0;
        if (next_line_info_it != func_info.line_info.end()) {
          line_info.size =
            next_line_info_it->rva_to_func - line_info.rva_to_func;
        } else {
          
          
          
          
          
          
          
          if (next_addr != 0) {
            ElfW(Off) next_addr_offset = next_addr - func_info.addr;
            line_info.size = next_addr_offset - line_info.rva_to_func;
          } else {
            line_info.size = kDefaultSize;
          }
        }
        line_info.rva_to_base = line_info.rva_to_func + func_info.rva_to_base;
      }  
    }  
  } 
  return true;
}

static bool LoadSymbols(const ElfW(Shdr) *stab_section,
                        const ElfW(Shdr) *stabstr_section,
                        ElfW(Addr) loading_addr,
                        struct SymbolInfo *symbols) {
  if (stab_section == NULL || stabstr_section == NULL)
    return false;

  struct nlist *lists =
    reinterpret_cast<struct nlist *>(stab_section->sh_offset);
  int nstab = stab_section->sh_size / sizeof(struct nlist);
  
  for (int i = 0; i < nstab; ) {
    int step = 1;
    struct nlist *cur_list = lists + i;
    if (cur_list->n_type == N_SO) {
      
      struct SourceFileInfo source_file_info;
      source_file_info.name_index = cur_list->n_un.n_strx;
      source_file_info.name = reinterpret_cast<char *>(cur_list->n_un.n_strx +
                                 stabstr_section->sh_offset);
      source_file_info.addr = cur_list->n_value;
      if (strchr(source_file_info.name, '.'))
        source_file_info.source_id = symbols->next_source_id++;
      else
        source_file_info.source_id = -1;
      step = LoadFuncSymbols(cur_list, lists + nstab,
                             stabstr_section, &source_file_info);
      symbols->source_file_info.push_back(source_file_info);
    }
    i += step;
  }

  
  if (ComputeSizeAndRVA(loading_addr, symbols)) {
    
    
    
    AddIncludedFiles(symbols, stabstr_section);
    return true;
  }
  return false;
}

static bool LoadSymbols(ElfW(Ehdr) *elf_header, struct SymbolInfo *symbols) {
  
  FixAddress(elf_header);
  ElfW(Addr) loading_addr = GetLoadingAddress(
      reinterpret_cast<ElfW(Phdr) *>(elf_header->e_phoff),
      elf_header->e_phnum);

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

  
  return LoadSymbols(stab_section, stabstr_section, loading_addr, symbols);
}

static bool WriteModuleInfo(FILE *file,
                            ElfW(Half) arch,
                            const std::string &obj_file) {
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
    char id_no_dash[40];
    int id_no_dash_len = 0;
    memset(id_no_dash, 0, sizeof(id_no_dash));
    for (int i = 0; identifier_str[i] != '\0'; ++i)
      if (identifier_str[i] != '-')
        id_no_dash[id_no_dash_len++] = identifier_str[i];
    
    id_no_dash[id_no_dash_len++] = '0';
    std::string filename = obj_file;
    size_t slash_pos = obj_file.find_last_of("/");
    if (slash_pos != std::string::npos)
      filename = obj_file.substr(slash_pos + 1);
    return 0 <= fprintf(file, "MODULE Linux %s %s %s\n", arch_name,
                        id_no_dash, filename.c_str());
  }
  return false;
}

static bool WriteSourceFileInfo(FILE *file, const struct SymbolInfo &symbols) {
  for (SourceFileInfoList::const_iterator it =
	 symbols.source_file_info.begin();
       it != symbols.source_file_info.end(); it++) {
    if (it->source_id != -1) {
      const char *name = it->name;
      if (0 > fprintf(file, "FILE %d %s\n", it->source_id, name))
        return false;
    }
  }
  return true;
}

static bool WriteOneFunction(FILE *file,
                             const struct FuncInfo &func_info){
  
  std::string func_name(func_info.name);
  std::string::size_type last_colon = func_name.find_last_of(':');
  if (last_colon != std::string::npos)
    func_name = func_name.substr(0, last_colon);
  func_name = Demangle(func_name.c_str());

  if (func_info.size <= 0)
    return true;

  if (0 <= fprintf(file, "FUNC %lx %lx %d %s\n",
                   (unsigned long) func_info.rva_to_base,
                   (unsigned long) func_info.size,
                   func_info.stack_param_size,
                   func_name.c_str())) {
    for (LineInfoList::const_iterator it = func_info.line_info.begin();
	 it != func_info.line_info.end(); it++) {
      const struct LineInfo &line_info = *it;
      if (0 > fprintf(file, "%lx %lx %d %d\n",
                      (unsigned long) line_info.rva_to_base,
                      (unsigned long) line_info.size,
                      line_info.line_num,
                      line_info.source_id))
        return false;
    }
    return true;
  }
  return false;
}

static bool WriteFunctionInfo(FILE *file, const struct SymbolInfo &symbols) {
  for (SourceFileInfoList::const_iterator it =
	 symbols.source_file_info.begin();
       it != symbols.source_file_info.end(); it++) {
    const struct SourceFileInfo &file_info = *it;
    for (FuncInfoList::const_iterator fiIt = file_info.func_info.begin(); 
	 fiIt != file_info.func_info.end(); fiIt++) {
      const struct FuncInfo &func_info = *fiIt;
      if (!WriteOneFunction(file, func_info))
        return false;
    }
  }
  return true;
}

static bool DumpStabSymbols(FILE *file, const struct SymbolInfo &symbols) {
  return WriteSourceFileInfo(file, symbols) &&
    WriteFunctionInfo(file, symbols);
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
  struct SymbolInfo symbols;
  symbols.next_source_id = 0;

  if (!LoadSymbols(elf_header, &symbols))
     return false;
  
  if (WriteModuleInfo(sym_file, elf_header->e_machine, obj_file) &&
      DumpStabSymbols(sym_file, symbols))
    return true;

  return false;
}

}  
