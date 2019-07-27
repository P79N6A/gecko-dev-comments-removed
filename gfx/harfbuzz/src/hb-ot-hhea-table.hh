

























#ifndef HB_OT_HHEA_TABLE_HH
#define HB_OT_HHEA_TABLE_HH

#include "hb-open-type-private.hh"


namespace OT {







#define HB_OT_TAG_hhea HB_TAG('h','h','e','a')
#define HB_OT_TAG_vhea HB_TAG('v','h','e','a')


struct _hea
{
  static const hb_tag_t tableTag = HB_TAG('_','h','e','a');

  static const hb_tag_t hheaTag	= HB_OT_TAG_hhea;
  static const hb_tag_t vheaTag	= HB_OT_TAG_vhea;

  inline bool sanitize (hb_sanitize_context_t *c) const
  {
    TRACE_SANITIZE (this);
    return TRACE_RETURN (c->check_struct (this) && likely (version.major == 1));
  }

  public:
  FixedVersion	version;		
  FWORD		ascender;		
  FWORD		descender;		
  FWORD		lineGap;		
  UFWORD	advanceMax;		

  FWORD		minLeadingBearing;	

  FWORD		minTrailingBearing;	


  FWORD		maxExtent;		

  SHORT		caretSlopeRise;		


  SHORT		caretSlopeRun;		
  SHORT		caretOffset;		




  SHORT		reserved1;		
  SHORT		reserved2;		
  SHORT		reserved3;		
  SHORT		reserved4;		
  SHORT		metricDataFormat;	
  USHORT	numberOfLongMetrics;	

  public:
  DEFINE_SIZE_STATIC (36);
};

struct hhea : _hea {
  static const hb_tag_t tableTag	= HB_OT_TAG_hhea;
};
struct vhea : _hea {
  static const hb_tag_t tableTag	= HB_OT_TAG_vhea;
};


} 


#endif 
