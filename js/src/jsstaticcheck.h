






































#ifndef jsstaticcheck_h___
#define jsstaticcheck_h___

#ifdef NS_STATIC_CHECKING



inline __attribute__ ((unused)) void MUST_FLOW_THROUGH(const char *label) {
}


#define MUST_FLOW_LABEL(label) goto label; label:
#else
#define MUST_FLOW_THROUGH(label) ((void)0)
#define MUST_FLOW_LABEL(label)
#endif

#endif 
