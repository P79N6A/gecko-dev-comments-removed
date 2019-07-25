struct dl_phdr_info {
        Elf_W(Addr)        dlpi_addr;  
         char   *dlpi_name;  

        const Elf_W(Phdr) *dlpi_phdr;  


        Elf_W(Half)        dlpi_phnum; 
};

int
dl_iterate_phdr (int (*callback) (struct dl_phdr_info *info, size_t size, void *data),
                 void *data);

