

























#ifndef HB_OT_HMTX_TABLE_HH
#define HB_OT_HMTX_TABLE_HH

#include "hb-open-type-private.hh"


namespace OT {






#define HB_OT_TAG_hmtx HB_TAG('h','m','t','x')


struct LongHorMetric
{
  USHORT	advanceWidth;
  SHORT		lsb;
  public:
  DEFINE_SIZE_STATIC (4);
};

struct hmtx
{
  static const hb_tag_t tableTag	= HB_OT_TAG_hmtx;

  inline bool sanitize (hb_sanitize_context_t *c) {
    TRACE_SANITIZE (this);
    

    return TRACE_RETURN (true);
  }

  public:
  LongHorMetric	longHorMetric[VAR];	







  SHORT		leftSideBearingX[VAR];	












  public:
  DEFINE_SIZE_ARRAY2 (0, longHorMetric, leftSideBearingX);
};


} 


#endif 
