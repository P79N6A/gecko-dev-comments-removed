

























#include "hb-set-private.hh"












hb_set_t *
hb_set_create (void)
{
  hb_set_t *set;

  if (!(set = hb_object_create<hb_set_t> ()))
    return hb_set_get_empty ();

  set->clear ();

  return set;
}








hb_set_t *
hb_set_get_empty (void)
{
  static const hb_set_t _hb_set_nil = {
    HB_OBJECT_HEADER_STATIC,
    true, 

    {0} 
  };

  return const_cast<hb_set_t *> (&_hb_set_nil);
}









hb_set_t *
hb_set_reference (hb_set_t *set)
{
  return hb_object_reference (set);
}







void
hb_set_destroy (hb_set_t *set)
{
  if (!hb_object_destroy (set)) return;

  set->fini ();

  free (set);
}













hb_bool_t
hb_set_set_user_data (hb_set_t           *set,
		      hb_user_data_key_t *key,
		      void *              data,
		      hb_destroy_func_t   destroy,
		      hb_bool_t           replace)
{
  return hb_object_set_user_data (set, key, data, destroy, replace);
}










void *
hb_set_get_user_data (hb_set_t           *set,
		      hb_user_data_key_t *key)
{
  return hb_object_get_user_data (set, key);
}












hb_bool_t
hb_set_allocation_successful (const hb_set_t  *set HB_UNUSED)
{
  return !set->in_error;
}









void
hb_set_clear (hb_set_t *set)
{
  set->clear ();
}











hb_bool_t
hb_set_is_empty (const hb_set_t *set)
{
  return set->is_empty ();
}












hb_bool_t
hb_set_has (const hb_set_t *set,
	    hb_codepoint_t  codepoint)
{
  return set->has (codepoint);
}










void
hb_set_add (hb_set_t       *set,
	    hb_codepoint_t  codepoint)
{
  set->add (codepoint);
}











void
hb_set_add_range (hb_set_t       *set,
		  hb_codepoint_t  first,
		  hb_codepoint_t  last)
{
  set->add_range (first, last);
}










void
hb_set_del (hb_set_t       *set,
	    hb_codepoint_t  codepoint)
{
  set->del (codepoint);
}











void
hb_set_del_range (hb_set_t       *set,
		  hb_codepoint_t  first,
		  hb_codepoint_t  last)
{
  set->del_range (first, last);
}












hb_bool_t
hb_set_is_equal (const hb_set_t *set,
		 const hb_set_t *other)
{
  return set->is_equal (other);
}










void
hb_set_set (hb_set_t       *set,
	    const hb_set_t *other)
{
  set->set (other);
}










void
hb_set_union (hb_set_t       *set,
	      const hb_set_t *other)
{
  set->union_ (other);
}










void
hb_set_intersect (hb_set_t       *set,
		  const hb_set_t *other)
{
  set->intersect (other);
}










void
hb_set_subtract (hb_set_t       *set,
		 const hb_set_t *other)
{
  set->subtract (other);
}










void
hb_set_symmetric_difference (hb_set_t       *set,
			     const hb_set_t *other)
{
  set->symmetric_difference (other);
}









void
hb_set_invert (hb_set_t *set)
{
  set->invert ();
}











unsigned int
hb_set_get_population (const hb_set_t *set)
{
  return set->get_population ();
}











hb_codepoint_t
hb_set_get_min (const hb_set_t *set)
{
  return set->get_min ();
}











hb_codepoint_t
hb_set_get_max (const hb_set_t *set)
{
  return set->get_max ();
}












hb_bool_t
hb_set_next (const hb_set_t *set,
	     hb_codepoint_t *codepoint)
{
  return set->next (codepoint);
}














hb_bool_t
hb_set_next_range (const hb_set_t *set,
		   hb_codepoint_t *first,
		   hb_codepoint_t *last)
{
  return set->next_range (first, last);
}
