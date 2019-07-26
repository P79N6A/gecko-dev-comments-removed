






#ifndef SkStreamHelpers_DEFINED
#define SkStreamHelpers_DEFINED

class SkAutoMalloc;
class SkStream;












size_t CopyStreamToStorage(SkAutoMalloc* storage, SkStream* stream);

#endif 
