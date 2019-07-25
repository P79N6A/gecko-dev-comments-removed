
























#include "libunwind_i.h"
#include "remote.h"

static inline int
intern_string (unw_addr_space_t as, unw_accessors_t *a,
	       unw_word_t addr, char *buf, size_t buf_len, void *arg)
{
  size_t i;
  int ret;

  for (i = 0; i < buf_len; ++i)
    {
      if ((ret = fetch8 (as, a, &addr, (int8_t *) buf + i, arg)) < 0)
	return ret;

      if (buf[i] == '\0')
	return 0;		
    }
  buf[buf_len - 1] = '\0';	
  return -UNW_ENOMEM;
}

static inline int
get_proc_name (unw_addr_space_t as, unw_word_t ip,
	       char *buf, size_t buf_len, unw_word_t *offp, void *arg)
{
  unw_accessors_t *a = unw_get_accessors (as);
  unw_proc_info_t pi;
  int ret;

  buf[0] = '\0';	

  ret = unwi_find_dynamic_proc_info (as, ip, &pi, 1, arg);
  if (ret == 0)
    {
      unw_dyn_info_t *di = pi.unwind_info;

      if (offp)
	*offp = ip - pi.start_ip;

      switch (di->format)
	{
	case UNW_INFO_FORMAT_DYNAMIC:
	  ret = intern_string (as, a, di->u.pi.name_ptr, buf, buf_len, arg);
	  break;

	case UNW_INFO_FORMAT_TABLE:
	case UNW_INFO_FORMAT_REMOTE_TABLE:
	  

	  ret = -UNW_ENOINFO;
	  break;

	default:
	  ret = -UNW_EINVAL;
	  break;
	}
      unwi_put_dynamic_unwind_info (as, &pi, arg);
      return ret;
    }

  if (ret != -UNW_ENOINFO)
    return ret;

  

  if (a->get_proc_name)
    return (*a->get_proc_name) (as, ip, buf, buf_len, offp, arg);

  return -UNW_ENOINFO;
}

PROTECTED int
unw_get_proc_name (unw_cursor_t *cursor, char *buf, size_t buf_len,
		   unw_word_t *offp)
{
  struct cursor *c = (struct cursor *) cursor;

  return get_proc_name (tdep_get_as (c), tdep_get_ip (c), buf, buf_len, offp,
			tdep_get_as_arg (c));
}
