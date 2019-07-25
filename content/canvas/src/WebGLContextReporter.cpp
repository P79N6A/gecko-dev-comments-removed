





































#include "WebGLContext.h"
#include "nsIMemoryReporter.h"

using namespace mozilla;


class WebGLMemoryMultiReporter MOZ_FINAL : public nsIMemoryMultiReporter 
{
  public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSIMEMORYMULTIREPORTER
};

NS_IMPL_ISUPPORTS1(WebGLMemoryMultiReporter, nsIMemoryMultiReporter)

NS_IMETHODIMP
WebGLMemoryMultiReporter::GetName(nsACString &aName)
{
  aName.AssignLiteral("webgl");
  return NS_OK;
}

NS_IMETHODIMP
WebGLMemoryMultiReporter::GetExplicitNonHeap(PRInt64 *aAmount)
{
    
    *aAmount = 0;
    return NS_OK;
}

NS_IMETHODIMP
WebGLMemoryMultiReporter::CollectReports(nsIMemoryMultiReporterCallback* aCb, 
                                         nsISupports* aClosure)
{
    aCb->Callback(
        EmptyCString(), 
        NS_LITERAL_CSTRING("webgl-texture-memory"),
        nsIMemoryReporter::KIND_OTHER, nsIMemoryReporter::UNITS_BYTES,
        WebGLMemoryMultiReporterWrapper::GetTextureMemoryUsed(),
        NS_LITERAL_CSTRING("Memory used by WebGL textures.The OpenGL"
            " implementation is free to store these textures in either video"
            " memory or main memory. This measurement is only a lower bound,"
            " actual memory usage may be higher for example if the storage" 
            " is strided."),  
        aClosure);
   
    aCb->Callback(
        EmptyCString(), 
        NS_LITERAL_CSTRING("webgl-texture-count"),
        nsIMemoryReporter::KIND_OTHER, nsIMemoryReporter::UNITS_COUNT,
         WebGLMemoryMultiReporterWrapper::GetTextureCount(), 
        NS_LITERAL_CSTRING("Number of WebGL textures."), 
        aClosure);

    aCb->Callback(
        EmptyCString(), 
        NS_LITERAL_CSTRING("webgl-buffer-memory"),
        nsIMemoryReporter::KIND_OTHER, nsIMemoryReporter::UNITS_BYTES,
         WebGLMemoryMultiReporterWrapper::GetBufferMemoryUsed(), 
        NS_LITERAL_CSTRING("Memory used by WebGL buffers. The OpenGL"
            " implementation is free to store these buffers in either video"
            " memory or main memory. This measurement is only a lower bound,"
            " actual memory usage may be higher for example if the storage"
            " is strided."),
        aClosure);

    aCb->Callback(
        EmptyCString(), 
        NS_LITERAL_CSTRING("explicit/webgl/buffer-cache-memory"),
        nsIMemoryReporter::KIND_HEAP, nsIMemoryReporter::UNITS_BYTES,
        WebGLMemoryMultiReporterWrapper::GetBufferCacheMemoryUsed(),
        NS_LITERAL_CSTRING("Memory used by WebGL buffer caches. The WebGL"
            " implementation caches the contents of element array buffers"
            " only.This adds up with the webgl-buffer-memory value, but"
            " contrary to it, this one represents bytes on the heap,"
            " not managed by OpenGL."),
        aClosure);

    aCb->Callback(
        EmptyCString(), NS_LITERAL_CSTRING("webgl-buffer-count"),
        nsIMemoryReporter::KIND_OTHER, nsIMemoryReporter::UNITS_COUNT,
        WebGLMemoryMultiReporterWrapper::GetBufferCount(),
        NS_LITERAL_CSTRING("Number of WebGL buffers."), 
        aClosure);
   
    aCb->Callback(
        EmptyCString(), 
        NS_LITERAL_CSTRING("webgl-renderbuffer-memory"),
        nsIMemoryReporter::KIND_OTHER, nsIMemoryReporter::UNITS_BYTES,
        WebGLMemoryMultiReporterWrapper::GetRenderbufferMemoryUsed(), 
        NS_LITERAL_CSTRING("Memory used by WebGL renderbuffers. The OpenGL"
            " implementation is free to store these renderbuffers in either"
            " video memory or main memory. This measurement is only a lower"
            " bound, actual memory usage may be higher for example if the"
            " storage is strided."),
        aClosure);
   
    aCb->Callback(
        EmptyCString(),
        NS_LITERAL_CSTRING("webgl-renderbuffer-count"),
        nsIMemoryReporter::KIND_OTHER, nsIMemoryReporter::UNITS_COUNT,
        WebGLMemoryMultiReporterWrapper::GetRenderbufferCount(),
        NS_LITERAL_CSTRING("Number of WebGL renderbuffers."),
        aClosure);
  
    aCb->Callback(
        EmptyCString(),
        NS_LITERAL_CSTRING("explicit/webgl/shader"),
        nsIMemoryReporter::KIND_HEAP, nsIMemoryReporter::UNITS_BYTES,
        WebGLMemoryMultiReporterWrapper::GetShaderSize(), 
        NS_LITERAL_CSTRING("Combined size of WebGL shader ASCII sources and"
            " translation logs cached on the heap."), 
        aClosure);

    aCb->Callback(
        EmptyCString(), 
        NS_LITERAL_CSTRING("webgl-shader-count"),
        nsIMemoryReporter::KIND_OTHER, nsIMemoryReporter::UNITS_COUNT,
        WebGLMemoryMultiReporterWrapper::GetShaderCount(), 
        NS_LITERAL_CSTRING("Number of WebGL shaders."), 
        aClosure);

    aCb->Callback(
        EmptyCString(), 
        NS_LITERAL_CSTRING("webgl-context-count"),
        nsIMemoryReporter::KIND_OTHER, nsIMemoryReporter::UNITS_COUNT,
        WebGLMemoryMultiReporterWrapper::GetContextCount(), 
        NS_LITERAL_CSTRING("Number of WebGL contexts."), 
        aClosure);

    return NS_OK;
}

WebGLMemoryMultiReporterWrapper* WebGLMemoryMultiReporterWrapper::sUniqueInstance = nsnull;

WebGLMemoryMultiReporterWrapper* WebGLMemoryMultiReporterWrapper::UniqueInstance()
{
    if (!sUniqueInstance) {
        sUniqueInstance = new WebGLMemoryMultiReporterWrapper;
    }
    return sUniqueInstance;     
}

WebGLMemoryMultiReporterWrapper::WebGLMemoryMultiReporterWrapper()
{ 
    mReporter = new WebGLMemoryMultiReporter;   
    NS_RegisterMemoryMultiReporter(mReporter);
}

WebGLMemoryMultiReporterWrapper::~WebGLMemoryMultiReporterWrapper()
{
    NS_UnregisterMemoryMultiReporter(mReporter);
}

NS_MEMORY_REPORTER_MALLOC_SIZEOF_FUN(WebGLBufferMallocSizeOfFun, "webgl-buffer")

PRInt64 
WebGLMemoryMultiReporterWrapper::GetBufferCacheMemoryUsed() {
    const ContextsArrayType & contexts = Contexts();
    PRInt64 result = 0;
    for (size_t i = 0; i < contexts.Length(); ++i) {
        for (size_t j = 0; j < contexts[i]->mBuffers.Length(); ++j) 
            if (contexts[i]->mBuffers[j]->Target() == LOCAL_GL_ELEMENT_ARRAY_BUFFER)
               result += contexts[i]->mBuffers[j]->SizeOfIncludingThis(WebGLBufferMallocSizeOfFun);
    }
    return result;
}

NS_MEMORY_REPORTER_MALLOC_SIZEOF_FUN(WebGLShaderMallocSizeOfFun, "webgl-shader")

PRInt64 
WebGLMemoryMultiReporterWrapper::GetShaderSize() {
    const ContextsArrayType & contexts = Contexts();
    PRInt64 result = 0;
    for (size_t i = 0; i < contexts.Length(); ++i) {
        for (size_t j = 0; j < contexts[i]->mShaders.Length(); ++j) 
            result += contexts[i]->mShaders[j]->SizeOfIncludingThis(WebGLShaderMallocSizeOfFun);
    }
    return result;
}
