






#ifndef SkStreamPriv_DEFINED
#define SkStreamPriv_DEFINED

class SkAutoMalloc;
class SkStream;
class SkStreamRewindable;
class SkData;












size_t SkCopyStreamToStorage(SkAutoMalloc* storage, SkStream* stream);








SkData *SkCopyStreamToData(SkStream* stream);







SkStreamRewindable* SkStreamRewindableFromSkStream(SkStream* stream);

#endif  
