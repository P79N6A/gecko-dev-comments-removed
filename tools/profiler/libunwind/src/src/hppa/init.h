
























#include "unwind_i.h"

static inline int
common_init (struct cursor *c, unsigned use_prev_instr)
{
  int ret;

  c->ip_loc = HPPA_REG_LOC (c, UNW_HPPA_IP);
  c->sp_loc = HPPA_REG_LOC (c, UNW_HPPA_SP);

  ret = hppa_get (c, c->ip_loc, &c->ip);
  if (ret < 0)
    return ret;

  ret = hppa_get (c, HPPA_REG_LOC (c, UNW_HPPA_SP), &c->sp);
  if (ret < 0)
    return ret;

  c->dwarf.stash_frames = 0;
  c->dwarf.use_prev_instr = use_prev_instr;
  return 0;
}
