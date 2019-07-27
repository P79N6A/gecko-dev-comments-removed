







#include "libGLESv2/renderer/d3d/d3d9/VertexDeclarationCache.h"
#include "libGLESv2/renderer/d3d/d3d9/VertexBuffer9.h"
#include "libGLESv2/renderer/d3d/d3d9/formatutils9.h"
#include "libGLESv2/ProgramBinary.h"
#include "libGLESv2/VertexAttribute.h"

namespace rx
{

VertexDeclarationCache::VertexDeclarationCache() : mMaxLru(0)
{
    for (int i = 0; i < NUM_VERTEX_DECL_CACHE_ENTRIES; i++)
    {
        mVertexDeclCache[i].vertexDeclaration = NULL;
        mVertexDeclCache[i].lruCount = 0;
    }

    for (int i = 0; i < gl::MAX_VERTEX_ATTRIBS; i++)
    {
        mAppliedVBs[i].serial = 0;
    }

    mLastSetVDecl = NULL;
    mInstancingEnabled = true;
}

VertexDeclarationCache::~VertexDeclarationCache()
{
    for (int i = 0; i < NUM_VERTEX_DECL_CACHE_ENTRIES; i++)
    {
        SafeRelease(mVertexDeclCache[i].vertexDeclaration);
    }
}

GLenum VertexDeclarationCache::applyDeclaration(IDirect3DDevice9 *device, TranslatedAttribute attributes[], gl::ProgramBinary *programBinary, GLsizei instances, GLsizei *repeatDraw)
{
    *repeatDraw = 1;

    int indexedAttribute = gl::MAX_VERTEX_ATTRIBS;
    int instancedAttribute = gl::MAX_VERTEX_ATTRIBS;

    if (instances == 0)
    {
        for (int i = 0; i < gl::MAX_VERTEX_ATTRIBS; ++i)
        {
            if (attributes[i].divisor != 0)
            {
                
                
                instances = 1;
                break;
            }
        }
    }

    if (instances > 0)
    {
        
        for (int i = 0; i < gl::MAX_VERTEX_ATTRIBS; i++)
        {
            if (attributes[i].active)
            {
                if (indexedAttribute == gl::MAX_VERTEX_ATTRIBS && attributes[i].divisor == 0)
                {
                    indexedAttribute = i;
                }
                else if (instancedAttribute == gl::MAX_VERTEX_ATTRIBS && attributes[i].divisor != 0)
                {
                    instancedAttribute = i;
                }
                if (indexedAttribute != gl::MAX_VERTEX_ATTRIBS && instancedAttribute != gl::MAX_VERTEX_ATTRIBS)
                    break;   
            }
        }

        if (indexedAttribute == gl::MAX_VERTEX_ATTRIBS)
        {
            return GL_INVALID_OPERATION;
        }
    }

    D3DCAPS9 caps;
    device->GetDeviceCaps(&caps);

    D3DVERTEXELEMENT9 elements[gl::MAX_VERTEX_ATTRIBS + 1];
    D3DVERTEXELEMENT9 *element = &elements[0];

    for (int i = 0; i < gl::MAX_VERTEX_ATTRIBS; i++)
    {
        if (attributes[i].active)
        {
            
            ASSERT(attributes[i].storage == NULL);

            int stream = i;

            if (instances > 0)
            {
                
                if (instancedAttribute == gl::MAX_VERTEX_ATTRIBS)
                {
                    *repeatDraw = instances;
                }
                else
                {
                    if (i == indexedAttribute)
                    {
                        stream = 0;
                    }
                    else if (i == 0)
                    {
                        stream = indexedAttribute;
                    }

                    UINT frequency = 1;
                    
                    if (attributes[i].divisor == 0)
                    {
                        frequency = D3DSTREAMSOURCE_INDEXEDDATA | instances;
                    }
                    else
                    {
                        frequency = D3DSTREAMSOURCE_INSTANCEDATA | attributes[i].divisor;
                    }
                    
                    device->SetStreamSourceFreq(stream, frequency);
                    mInstancingEnabled = true;
                }
            }

            VertexBuffer9 *vertexBuffer = VertexBuffer9::makeVertexBuffer9(attributes[i].vertexBuffer);

            if (mAppliedVBs[stream].serial != attributes[i].serial ||
                mAppliedVBs[stream].stride != attributes[i].stride ||
                mAppliedVBs[stream].offset != attributes[i].offset)
            {
                device->SetStreamSource(stream, vertexBuffer->getBuffer(), attributes[i].offset, attributes[i].stride);
                mAppliedVBs[stream].serial = attributes[i].serial;
                mAppliedVBs[stream].stride = attributes[i].stride;
                mAppliedVBs[stream].offset = attributes[i].offset;
            }

            gl::VertexFormat vertexFormat(*attributes[i].attribute, GL_FLOAT);
            const d3d9::VertexFormat &d3d9VertexInfo = d3d9::GetVertexFormatInfo(caps.DeclTypes, vertexFormat);

            element->Stream = stream;
            element->Offset = 0;
            element->Type = d3d9VertexInfo.nativeFormat;
            element->Method = D3DDECLMETHOD_DEFAULT;
            element->Usage = D3DDECLUSAGE_TEXCOORD;
            element->UsageIndex = programBinary->getSemanticIndex(i);
            element++;
        }
    }

    if (instances == 0 || instancedAttribute == gl::MAX_VERTEX_ATTRIBS)
    {
        if (mInstancingEnabled)
        {
            for (int i = 0; i < gl::MAX_VERTEX_ATTRIBS; i++)
            {
                device->SetStreamSourceFreq(i, 1);
            }

            mInstancingEnabled = false;
        }
    }

    static const D3DVERTEXELEMENT9 end = D3DDECL_END();
    *(element++) = end;

    for (int i = 0; i < NUM_VERTEX_DECL_CACHE_ENTRIES; i++)
    {
        VertexDeclCacheEntry *entry = &mVertexDeclCache[i];
        if (memcmp(entry->cachedElements, elements, (element - elements) * sizeof(D3DVERTEXELEMENT9)) == 0 && entry->vertexDeclaration)
        {
            entry->lruCount = ++mMaxLru;
            if(entry->vertexDeclaration != mLastSetVDecl)
            {
                device->SetVertexDeclaration(entry->vertexDeclaration);
                mLastSetVDecl = entry->vertexDeclaration;
            }

            return GL_NO_ERROR;
        }
    }

    VertexDeclCacheEntry *lastCache = mVertexDeclCache;

    for (int i = 0; i < NUM_VERTEX_DECL_CACHE_ENTRIES; i++)
    {
        if (mVertexDeclCache[i].lruCount < lastCache->lruCount)
        {
            lastCache = &mVertexDeclCache[i];
        }
    }

    if (lastCache->vertexDeclaration != NULL)
    {
        SafeRelease(lastCache->vertexDeclaration);
        
        
    }

    memcpy(lastCache->cachedElements, elements, (element - elements) * sizeof(D3DVERTEXELEMENT9));
    device->CreateVertexDeclaration(elements, &lastCache->vertexDeclaration);
    device->SetVertexDeclaration(lastCache->vertexDeclaration);
    mLastSetVDecl = lastCache->vertexDeclaration;
    lastCache->lruCount = ++mMaxLru;

    return GL_NO_ERROR;
}

void VertexDeclarationCache::markStateDirty()
{
    for (int i = 0; i < gl::MAX_VERTEX_ATTRIBS; i++)
    {
        mAppliedVBs[i].serial = 0;
    }

    mLastSetVDecl = NULL;
    mInstancingEnabled = true;   
}

}
