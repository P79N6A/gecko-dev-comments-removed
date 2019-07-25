























#ifndef dwarf_config_h
#define dwarf_config_h



#define DWARF_NUM_PRESERVED_REGS	128


#define DWARF_REGNUM_MAP_LENGTH		16


#define dwarf_is_big_endian(addr_space)	0



#define dwarf_to_cursor(c)	((unw_cursor_t *) (c))

typedef struct dwarf_loc
  {
    unw_word_t val;
#ifndef UNW_LOCAL_ONLY
    unw_word_t type;		
#endif
  }
dwarf_loc_t;

#endif 
