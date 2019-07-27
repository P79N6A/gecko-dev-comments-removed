




#ifndef WEBGLMEMORYTRACKER_H_
#define WEBGLMEMORYTRACKER_H_

#include "WebGLContext.h"
#include "WebGLBuffer.h"
#include "WebGLVertexAttribData.h"
#include "WebGLShader.h"
#include "WebGLProgram.h"
#include "WebGLUniformLocation.h"
#include "WebGLTexture.h"
#include "WebGLRenderbuffer.h"
#include "mozilla/StaticPtr.h"
#include "nsIMemoryReporter.h"

namespace mozilla {

class WebGLMemoryTracker : public nsIMemoryReporter
{
    NS_DECL_THREADSAFE_ISUPPORTS
    NS_DECL_NSIMEMORYREPORTER

    WebGLMemoryTracker();
    static StaticRefPtr<WebGLMemoryTracker> sUniqueInstance;

    
    
    
    typedef nsTArray<const WebGLContext*> ContextsArrayType;
    ContextsArrayType mContexts;

    void InitMemoryReporter();

    static WebGLMemoryTracker* UniqueInstance();

    static ContextsArrayType & Contexts() { return UniqueInstance()->mContexts; }

    friend class WebGLContext;

  public:

    static void AddWebGLContext(const WebGLContext* c) {
        Contexts().AppendElement(c);
    }

    static void RemoveWebGLContext(const WebGLContext* c) {
        ContextsArrayType & contexts = Contexts();
        contexts.RemoveElement(c);
        if (contexts.IsEmpty()) {
            sUniqueInstance = nullptr;
        }
    }

  private:
    virtual ~WebGLMemoryTracker();

    static int64_t GetTextureMemoryUsed() {
        const ContextsArrayType & contexts = Contexts();
        int64_t result = 0;
        for(size_t i = 0; i < contexts.Length(); ++i) {
            for (const WebGLTexture *texture = contexts[i]->mTextures.getFirst();
                 texture;
                 texture = texture->getNext())
            {
                result += texture->MemoryUsage();
            }
        }
        return result;
    }

    static int64_t GetTextureCount() {
        const ContextsArrayType & contexts = Contexts();
        int64_t result = 0;
        for(size_t i = 0; i < contexts.Length(); ++i) {
            for (const WebGLTexture *texture = contexts[i]->mTextures.getFirst();
                 texture;
                 texture = texture->getNext())
            {
                result++;
            }
        }
        return result;
    }

    static int64_t GetBufferMemoryUsed() {
        const ContextsArrayType & contexts = Contexts();
        int64_t result = 0;
        for(size_t i = 0; i < contexts.Length(); ++i) {
            for (const WebGLBuffer *buffer = contexts[i]->mBuffers.getFirst();
                 buffer;
                 buffer = buffer->getNext())
            {
                result += buffer->ByteLength();
            }
        }
        return result;
    }

    static int64_t GetBufferCacheMemoryUsed();

    static int64_t GetBufferCount() {
        const ContextsArrayType & contexts = Contexts();
        int64_t result = 0;
        for(size_t i = 0; i < contexts.Length(); ++i) {
            for (const WebGLBuffer *buffer = contexts[i]->mBuffers.getFirst();
                 buffer;
                 buffer = buffer->getNext())
            {
                result++;
            }
        }
        return result;
    }

    static int64_t GetRenderbufferMemoryUsed() {
        const ContextsArrayType & contexts = Contexts();
        int64_t result = 0;
        for(size_t i = 0; i < contexts.Length(); ++i) {
            for (const WebGLRenderbuffer *rb = contexts[i]->mRenderbuffers.getFirst();
                 rb;
                 rb = rb->getNext())
            {
                result += rb->MemoryUsage();
            }
        }
        return result;
    }

    static int64_t GetRenderbufferCount() {
        const ContextsArrayType & contexts = Contexts();
        int64_t result = 0;
        for(size_t i = 0; i < contexts.Length(); ++i) {
            for (const WebGLRenderbuffer *rb = contexts[i]->mRenderbuffers.getFirst();
                 rb;
                 rb = rb->getNext())
            {
                result++;
            }
        }
        return result;
    }

    static int64_t GetShaderSize();

    static int64_t GetShaderCount() {
        const ContextsArrayType & contexts = Contexts();
        int64_t result = 0;
        for(size_t i = 0; i < contexts.Length(); ++i) {
            for (const WebGLShader *shader = contexts[i]->mShaders.getFirst();
                 shader;
                 shader = shader->getNext())
            {
                result++;
            }
        }
        return result;
    }

    static int64_t GetContextCount() {
        return Contexts().Length();
    }
};


} 

#endif
