
























#ifndef dwarf_config_h
#define dwarf_config_h


#define dwarf_to_unw_regnum(reg)			\
  (((reg) < DWARF_NUM_PRESERVED_REGS) ? (reg) : 0)




#define DWARF_NUM_PRESERVED_REGS	89


#define dwarf_is_big_endian(addr_space)	1



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
