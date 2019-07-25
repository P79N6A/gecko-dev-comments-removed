
























#include "unwind_i.h"



static inline int
rotate_gr (struct cursor *c, int reg)
{
  unsigned int rrb_gr, sor;
  int preg;

  sor = 8 * ((c->cfm >> 14) & 0xf);
  rrb_gr = (c->cfm >> 18) & 0x7f;

  if ((unsigned) (reg - 32) >= sor)
    preg = reg;
  else
    {
      preg = reg + rrb_gr;	
      if ((unsigned) (preg - 32) >= sor)
	preg -= sor;		
    }
  if (sor)
    Debug (15, "sor=%u rrb.gr=%u, r%d -> r%d\n", sor, rrb_gr, reg, preg);
  return preg;
}




static inline int
rotate_fr (struct cursor *c, int reg)
{
  unsigned int rrb_fr;
  int preg;

  rrb_fr = (c->cfm >> 25) & 0x7f;
  if (reg < 32)
    preg = reg;		
  else
    {
      preg = reg + rrb_fr;	
      if (preg > 127)
	preg -= 96;		
    }
  if (rrb_fr)
    Debug (15, "rrb.fr=%u, f%d -> f%d\n", rrb_fr, reg, preg);
  return preg;
}
