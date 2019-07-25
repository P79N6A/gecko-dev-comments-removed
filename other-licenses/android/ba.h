



























#ifndef __LINKER_BA_H
#define __LINKER_BA_H

struct ba_bits {
    unsigned allocated:1;           
    unsigned order:7;               
};

struct ba {
    
    unsigned long base;
    
    unsigned long size;
    
    unsigned long min_alloc;
    
    unsigned long max_order;
    
    int num_entries;
    

    struct ba_bits *bitmap;
};

extern void ba_init(struct ba *ba);
extern int ba_allocate(struct ba *ba, unsigned long len);
extern int ba_free(struct ba *ba, int index);
extern unsigned long ba_start_addr(struct ba *ba, int index);
extern unsigned long ba_len(struct ba *ba, int index);

#endif
