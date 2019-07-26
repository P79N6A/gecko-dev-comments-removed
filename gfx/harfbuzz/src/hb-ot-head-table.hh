



























#ifndef HB_OT_HEAD_TABLE_HH
#define HB_OT_HEAD_TABLE_HH

#include "hb-open-type-private.hh"


namespace OT {






#define HB_OT_TAG_head HB_TAG('h','e','a','d')

struct head
{
  static const hb_tag_t Tag	= HB_OT_TAG_head;

  inline unsigned int get_upem (void) const {
    unsigned int upem = unitsPerEm;
    
    return 16 <= upem && upem <= 16384 ? upem : 1000;
  }

  inline bool sanitize (hb_sanitize_context_t *c) {
    TRACE_SANITIZE (this);
    return TRACE_RETURN (c->check_struct (this) && likely (version.major == 1));
  }

  protected:
  FixedVersion	version;		

  FixedVersion	fontRevision;		
  ULONG		checkSumAdjustment;	


  ULONG		magicNumber;		
  USHORT	flags;			













































  USHORT	unitsPerEm;		


  LONGDATETIME	created;		

  LONGDATETIME	modified;		

  SHORT		xMin;			
  SHORT		yMin;			
  SHORT		xMax;			
  SHORT		yMax;			
  USHORT	macStyle;		







  USHORT	lowestRecPPEM;		
  SHORT		fontDirectionHint;	





  SHORT		indexToLocFormat;	
  SHORT		glyphDataFormat;	
  public:
  DEFINE_SIZE_STATIC (54);
};


} 


#endif 
