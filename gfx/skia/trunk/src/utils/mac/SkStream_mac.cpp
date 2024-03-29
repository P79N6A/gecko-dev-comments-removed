






#include "SkCGUtils.h"
#include "SkStream.h"



static void unref_proc(void* info, const void* addr, size_t size) {
    SkASSERT(info);
    ((SkRefCnt*)info)->unref();
}



static size_t get_bytes_proc(void* info, void* buffer, size_t bytes) {
    SkASSERT(info);
    return ((SkStream*)info)->read(buffer, bytes);
}

static off_t skip_forward_proc(void* info, off_t bytes) {
    return ((SkStream*)info)->skip((size_t) bytes);
}

static void rewind_proc(void* info) {
    SkASSERT(info);
    ((SkStream*)info)->rewind();
}

static void release_info_proc(void* info) {
    SkASSERT(info);
    ((SkStream*)info)->unref();
}

CGDataProviderRef SkCreateDataProviderFromStream(SkStream* stream) {
    stream->ref();  

    const void* addr = stream->getMemoryBase();
    if (addr) {
        
        return CGDataProviderCreateWithData(stream, addr, stream->getLength(),
                                            unref_proc);
    }

    CGDataProviderSequentialCallbacks rec;
    sk_bzero(&rec, sizeof(rec));
    rec.version = 0;
    rec.getBytes = get_bytes_proc;
    rec.skipForward = skip_forward_proc;
    rec.rewind = rewind_proc;
    rec.releaseInfo = release_info_proc;
    return CGDataProviderCreateSequential(stream, &rec);
}



#include "SkData.h"

CGDataProviderRef SkCreateDataProviderFromData(SkData* data) {
    data->ref();
    return CGDataProviderCreateWithData(data, data->data(), data->size(),
                                            unref_proc);
}
