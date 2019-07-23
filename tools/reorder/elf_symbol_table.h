




































#ifndef elf_symbol_table_h__
#define elf_symbol_table_h__







#include <elf.h>
#include <sys/mman.h>

#include "interval_map.h"

class elf_symbol_table {
protected:
    int              m_fd;
    void            *m_mapping;
    size_t           m_size;
    const char      *m_strtab;
    const Elf32_Sym *m_symbols;
    int              m_nsymbols;
    int              m_text_shndx;

    typedef interval_map<unsigned int, const Elf32_Sym *> rsymtab_t;
    rsymtab_t        m_rsymtab;

    int verify_elf_header(const Elf32_Ehdr *hdr);
    int finish();

public:
    elf_symbol_table()
        : m_fd(-1),
          m_mapping(MAP_FAILED),
          m_size(0),
          m_strtab(0),
          m_symbols(0),
          m_nsymbols(0),
          m_text_shndx(0) {}

    ~elf_symbol_table() { finish(); }

    





    int init(const char *file);

    




    const Elf32_Sym *lookup(unsigned int addr) const;

    


    const char *get_symbol_name(const Elf32_Sym *sym) const;

    


    bool is_function(const Elf32_Sym *sym) const {
        return (sym->st_size > 0) &&
            (ELF32_ST_TYPE(sym->st_info) == STT_FUNC) &&
            (sym->st_shndx == m_text_shndx); }

    typedef const Elf32_Sym *const_iterator;

    const_iterator begin() const { return const_iterator(m_symbols); }
    const_iterator end() const { return const_iterator(m_symbols + m_nsymbols); }
};

#endif 
