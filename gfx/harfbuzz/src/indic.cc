

























#include "hb-ot-shape-complex-indic-private.hh"

int
main (void)
{
  hb_unicode_funcs_t *funcs = hb_unicode_funcs_get_default ();

  printf ("There are split matras without a Unicode decomposition:\n");
  for (hb_codepoint_t u = 0; u < 0x110000; u++)
  {
    unsigned int type = get_indic_categories (u);

    unsigned int category = type & 0x0F;
    unsigned int position = type >> 4;

    hb_unicode_general_category_t cat = hb_unicode_general_category (funcs, u);
    unsigned int ccc = hb_unicode_combining_class (funcs, u);
    if (category == OT_M && ccc)
      printf ("U+%04X %d\n", u, ccc);




  }
}
