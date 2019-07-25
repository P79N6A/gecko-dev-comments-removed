

























#ifndef HB_SHAPER_LIST_HH
#define HB_SHAPER_LIST_HH
#endif  


#ifdef HAVE_GRAPHITE2
HB_SHAPER_IMPLEMENT (graphite2)
#endif
#ifdef HAVE_UNISCRIBE
HB_SHAPER_IMPLEMENT (uniscribe)
#endif
#ifdef HAVE_CORETEXT
HB_SHAPER_IMPLEMENT (coretext)
#endif

#ifdef HAVE_OT
HB_SHAPER_IMPLEMENT (ot) 
#endif

#ifdef HAVE_HB_OLD
HB_SHAPER_IMPLEMENT (old)
#endif

HB_SHAPER_IMPLEMENT (fallback) 
