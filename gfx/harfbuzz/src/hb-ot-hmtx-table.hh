

























#ifndef HB_OT_HMTX_TABLE_HH
#define HB_OT_HMTX_TABLE_HH

#include "hb-open-type-private.hh"


namespace OT {







#define HB_OT_TAG_hmtx HB_TAG('h','m','t','x')
#define HB_OT_TAG_vmtx HB_TAG('v','m','t','x')


struct LongMetric
{
  USHORT	advance; 
  SHORT		lsb; 
  public:
  DEFINE_SIZE_STATIC (4);
};

struct _mtx
{
  static const hb_tag_t tableTag = HB_TAG('_','m','t','x');

  static const hb_tag_t hmtxTag	= HB_OT_TAG_hmtx;
  static const hb_tag_t vmtxTag	= HB_OT_TAG_vmtx;

  inline bool sanitize (hb_sanitize_context_t *c) const
  {
    TRACE_SANITIZE (this);
    

    return TRACE_RETURN (true);
  }

  public:
  LongMetric	longMetric[VAR];	







  SHORT		leadingBearingX[VAR];	












  public:
  DEFINE_SIZE_ARRAY2 (0, longMetric, leadingBearingX);
};

struct hmtx : _mtx {
  static const hb_tag_t tableTag	= HB_OT_TAG_hmtx;
};
struct vmtx : _mtx {
  static const hb_tag_t tableTag	= HB_OT_TAG_vmtx;
};

} 


#endif 
