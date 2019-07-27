

























#ifndef HB_OT_HHEA_TABLE_HH
#define HB_OT_HHEA_TABLE_HH

#include "hb-open-type-private.hh"


namespace OT {






#define HB_OT_TAG_hhea HB_TAG('h','h','e','a')


struct hhea
{
  static const hb_tag_t tableTag	= HB_OT_TAG_hhea;

  inline bool sanitize (hb_sanitize_context_t *c) {
    TRACE_SANITIZE (this);
    return TRACE_RETURN (c->check_struct (this) && likely (version.major == 1));
  }

  public:
  FixedVersion	version;		
  FWORD		ascender;		



  FWORD		descender;		



  FWORD		lineGap;		



  UFWORD	advanceWidthMax;	

  FWORD		minLeftSideBearing;	

  FWORD		minRightSideBearing;	


  FWORD		xMaxExtent;		
  SHORT		caretSlopeRise;		

  SHORT		caretSlopeRun;		
  SHORT		caretOffset;		




  SHORT		reserved1;		
  SHORT		reserved2;		
  SHORT		reserved3;		
  SHORT		reserved4;		
  SHORT		metricDataFormat;	
  USHORT	numberOfHMetrics;	

  public:
  DEFINE_SIZE_STATIC (36);
};


} 


#endif 
