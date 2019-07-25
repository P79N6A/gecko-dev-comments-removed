
























#ifndef libunwind_ptrace_h
#define libunwind_ptrace_h

#include <libunwind.h>

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif






extern void *_UPT_create (pid_t);
extern void _UPT_destroy (void *);
extern int _UPT_find_proc_info (unw_addr_space_t, unw_word_t,
				unw_proc_info_t *, int, void *);
extern void _UPT_put_unwind_info (unw_addr_space_t, unw_proc_info_t *, void *);
extern int _UPT_get_dyn_info_list_addr (unw_addr_space_t, unw_word_t *,
					void *);
extern int _UPT_access_mem (unw_addr_space_t, unw_word_t, unw_word_t *, int,
			    void *);
extern int _UPT_access_reg (unw_addr_space_t, unw_regnum_t, unw_word_t *,
			    int, void *);
extern int _UPT_access_fpreg (unw_addr_space_t, unw_regnum_t, unw_fpreg_t *,
			      int, void *);
extern int _UPT_get_proc_name (unw_addr_space_t, unw_word_t, char *, size_t,
			       unw_word_t *, void *);
extern int _UPT_resume (unw_addr_space_t, unw_cursor_t *, void *);
extern unw_accessors_t _UPT_accessors;


#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

#endif
