

























#ifndef HB_OT_MAXP_TABLE_HH
#define HB_OT_MAXP_TABLE_HH

#include "hb-open-type-private.hh"


namespace OT {






#define HB_OT_TAG_maxp HB_TAG('m','a','x','p')

struct maxp
{
  static const hb_tag_t tableTag	= HB_OT_TAG_maxp;

  inline unsigned int get_num_glyphs (void) const {
    return numGlyphs;
  }

  inline bool sanitize (hb_sanitize_context_t *c) {
    TRACE_SANITIZE (this);
    return TRACE_RETURN (c->check_struct (this) &&
			 likely (version.major == 1 || (version.major == 0 && version.minor == 0x5000u)));
  }

  
  protected:
  FixedVersion	version;		

  USHORT	numGlyphs;		
  public:
  DEFINE_SIZE_STATIC (6);
};


} 


#endif 
