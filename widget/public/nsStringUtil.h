







































#ifndef NS_STR_UTIL_H
#define NS_STR_UTIL_H








#define NS_ALLOC_CHAR_BUF(aBuf, aSize, aActualSize) \
  int _ns_tmpActualSize = aActualSize;              \
  char _ns_smallBuffer[aSize];                      \
  char * const aBuf = _ns_tmpActualSize <= aSize ? _ns_smallBuffer : new char[_ns_tmpActualSize]; 

#define NS_FREE_CHAR_BUF(aBuf)   \
  if (aBuf != _ns_smallBuffer) \
   delete[] aBuf;

#endif 
