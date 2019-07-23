



































#ifndef def_get_h___
#define def_get_h___


#define GET_TEST(_class)                     \
extern "C" NS_EXPORT nsresult                \
getOJIAPITest(nsIOJIAPITest** test) {        \
  *test =  (nsIOJIAPITest*) new _class();    \
  return NS_OK;                              \
}                                           


#endif 

