




































#if !defined(XPCONNECT_STANDALONE)

#include "nsAutoPtr.h"
#include "nsScriptLoader.h"

#include "jsapi.h"
#include "jsxdrapi.h"

#include "mozilla/scache/StartupCache.h"
#include "mozilla/scache/StartupCacheUtils.h"

using namespace mozilla::scache;

static nsresult
ReadScriptFromStream(JSContext *cx, nsIObjectInputStream *stream,
                     JSObject **scriptObj)
{
    *scriptObj = nsnull;

    PRUint32 size;
    nsresult rv = stream->Read32(&size);
    NS_ENSURE_SUCCESS(rv, rv);

    char *data;
    rv = stream->ReadBytes(size, &data);
    NS_ENSURE_SUCCESS(rv, rv);

    JSXDRState *xdr = JS_XDRNewMem(cx, JSXDR_DECODE);
    NS_ENSURE_TRUE(xdr, NS_ERROR_OUT_OF_MEMORY);

    xdr->userdata = stream;
    JS_XDRMemSetData(xdr, data, size);

    if (!JS_XDRScriptObject(xdr, scriptObj)) {
        rv = NS_ERROR_FAILURE;
    }

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    

    uint32 length;
    data = static_cast<char*>(JS_XDRMemGetData(xdr, &length));
    JS_XDRMemSetData(xdr, nsnull, 0);
    JS_XDRDestroy(xdr);

    
    
    nsMemory::Free(data);

    return rv;
}

static nsresult
WriteScriptToStream(JSContext *cx, JSObject *scriptObj,
                    nsIObjectOutputStream *stream)
{
    JSXDRState *xdr = JS_XDRNewMem(cx, JSXDR_ENCODE);
    NS_ENSURE_TRUE(xdr, NS_ERROR_OUT_OF_MEMORY);

    xdr->userdata = stream;
    nsresult rv = NS_OK;

    if (JS_XDRScriptObject(xdr, &scriptObj)) {
        
        
        
        
        
        
        
        
        
        
        
        
        

        uint32 size;
        const char* data = reinterpret_cast<const char*>
                                           (JS_XDRMemGetData(xdr, &size));
        NS_ASSERTION(data, "no decoded JSXDRState data!");

        rv = stream->Write32(size);
        if (NS_SUCCEEDED(rv)) {
            rv = stream->WriteBytes(data, size);
        }
    } else {
        rv = NS_ERROR_FAILURE; 
    }

    JS_XDRDestroy(xdr);
    return rv;
}

nsresult
ReadCachedScript(StartupCache* cache, nsACString &uri, JSContext *cx, JSObject **scriptObj)
{
    nsresult rv;

    nsAutoArrayPtr<char> buf;
    PRUint32 len;
    rv = cache->GetBuffer(PromiseFlatCString(uri).get(), getter_Transfers(buf),
                          &len);
    if (NS_FAILED(rv)) {
        return rv; 
    }

    nsCOMPtr<nsIObjectInputStream> ois;
    rv = NS_NewObjectInputStreamFromBuffer(buf, len, getter_AddRefs(ois));
    NS_ENSURE_SUCCESS(rv, rv);
    buf.forget();

    return ReadScriptFromStream(cx, ois, scriptObj);
}

nsresult
WriteCachedScript(StartupCache* cache, nsACString &uri, JSContext *cx, JSObject *scriptObj)
{
    nsresult rv;

    nsCOMPtr<nsIObjectOutputStream> oos;
    nsCOMPtr<nsIStorageStream> storageStream;
    rv = NS_NewObjectOutputWrappedStorageStream(getter_AddRefs(oos),
                                                getter_AddRefs(storageStream),
                                                true);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = WriteScriptToStream(cx, scriptObj, oos);
    oos->Close();
    NS_ENSURE_SUCCESS(rv, rv);

    nsAutoArrayPtr<char> buf;
    PRUint32 len;
    rv = NS_NewBufferFromStorageStream(storageStream, getter_Transfers(buf),
                                       &len);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = cache->PutBuffer(PromiseFlatCString(uri).get(), buf, len);
    return rv;
}

#endif 
