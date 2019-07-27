





#include "LayerScope.h"

#include "Composer2D.h"
#include "Effects.h"
#include "mozilla/TimeStamp.h"
#include "mozilla/Preferences.h"
#include "mozilla/Endian.h"
#include "TexturePoolOGL.h"
#include "mozilla/layers/CompositorOGL.h"
#include "mozilla/layers/LayerManagerComposite.h"
#include "mozilla/layers/TextureHostOGL.h"

#include "gfxColor.h"
#include "gfxContext.h"
#include "gfxUtils.h"
#include "gfxPrefs.h"
#include "nsIWidget.h"

#include "GLContext.h"
#include "GLContextProvider.h"
#include "GLReadTexImageHelper.h"

#include "nsIServiceManager.h"
#include "nsIConsoleService.h"

#include <memory>
#include "mozilla/LinkedList.h"
#include "mozilla/Base64.h"
#include "mozilla/SHA1.h"
#include "mozilla/StaticPtr.h"
#include "nsThreadUtils.h"
#include "nsISocketTransport.h"
#include "nsIServerSocket.h"
#include "nsReadLine.h"
#include "nsNetCID.h"
#include "nsIOutputStream.h"
#include "nsIAsyncInputStream.h"
#include "nsIEventTarget.h"
#include "nsProxyRelease.h"


#undef compress
#include "mozilla/Compression.h"


#include "protobuf/LayerScopePacket.pb.h"

namespace mozilla {
namespace layers {

using namespace mozilla::Compression;
using namespace mozilla::gfx;
using namespace mozilla::gl;
using namespace mozilla;
using namespace layerscope;

class DebugDataSender;
class DebugGLData;




class LayerScopeWebSocketHandler : public nsIInputStreamCallback {
public:
    NS_DECL_THREADSAFE_ISUPPORTS

    enum SocketStateType {
        NoHandshake,
        HandshakeSuccess,
        HandshakeFailed
    };

    LayerScopeWebSocketHandler()
        : mState(NoHandshake)
    { }

private:
    virtual ~LayerScopeWebSocketHandler()
    {
        if (mTransport) {
            mTransport->Close(NS_OK);
        }
    }

public:
    void OpenStream(nsISocketTransport* aTransport) {
        MOZ_ASSERT(aTransport);

        mTransport = aTransport;
        mTransport->OpenOutputStream(nsITransport::OPEN_BLOCKING,
                                     0,
                                     0,
                                     getter_AddRefs(mOutputStream));

        nsCOMPtr<nsIInputStream> debugInputStream;
        mTransport->OpenInputStream(0,
                                    0,
                                    0,
                                    getter_AddRefs(debugInputStream));
        mInputStream = do_QueryInterface(debugInputStream);
        mInputStream->AsyncWait(this, 0, 0, NS_GetCurrentThread());
    }

    bool WriteToStream(void *ptr, uint32_t size) {
        if (mState == NoHandshake) {
            
            
            return true;
        } else if (mState == HandshakeFailed) {
            return false;
        }

        
        uint8_t wsHeader[10];
        int wsHeaderSize = 0;
        const uint8_t opcode = 0x2;
        wsHeader[0] = 0x80 | (opcode & 0x0f); 
        if (size <= 125) {
            wsHeaderSize = 2;
            wsHeader[1] = size;
        } else if (size < 65536) {
            wsHeaderSize = 4;
            wsHeader[1] = 0x7E;
            NetworkEndian::writeUint16(wsHeader + 2, size);
        } else {
            wsHeaderSize = 10;
            wsHeader[1] = 0x7F;
            NetworkEndian::writeUint64(wsHeader + 2, size);
        }

        
        nsresult rv;
        uint32_t cnt;
        rv = mOutputStream->Write(reinterpret_cast<char*>(wsHeader),
                                 wsHeaderSize, &cnt);
        if (NS_FAILED(rv))
            return false;

        uint32_t written = 0;
        while (written < size) {
            uint32_t cnt;
            rv = mOutputStream->Write(reinterpret_cast<char*>(ptr) + written,
                                     size - written, &cnt);
            if (NS_FAILED(rv))
                return false;

            written += cnt;
        }

        return true;
    }

    
    NS_IMETHODIMP OnInputStreamReady(nsIAsyncInputStream *stream) MOZ_OVERRIDE
    {
        nsTArray<nsCString> protocolString;
        ReadInputStreamData(protocolString);

        if (WebSocketHandshake(protocolString)) {
            mState = HandshakeSuccess;
        } else {
            mState = HandshakeFailed;
        }
        return NS_OK;
    }
private:
    void ReadInputStreamData(nsTArray<nsCString>& aProtocolString)
    {
        nsLineBuffer<char> lineBuffer;
        nsCString line;
        bool more = true;
        do {
            NS_ReadLine(mInputStream.get(), &lineBuffer, line, &more);

            if (line.Length() > 0) {
                aProtocolString.AppendElement(line);
            }
        } while (more && line.Length() > 0);
    }

    bool WebSocketHandshake(nsTArray<nsCString>& aProtocolString)
    {
        nsresult rv;
        bool isWebSocket = false;
        nsCString version;
        nsCString wsKey;
        nsCString protocol;

        
        if (aProtocolString.Length() == 0)
            return false;

        
        const char* HTTP_METHOD = "GET ";
        if (strncmp(aProtocolString[0].get(), HTTP_METHOD, strlen(HTTP_METHOD)) != 0) {
            return false;
        }

        for (uint32_t i = 1; i < aProtocolString.Length(); ++i) {
            const char* line = aProtocolString[i].get();
            const char* prop_pos = strchr(line, ':');
            if (prop_pos != nullptr) {
                nsCString key(line, prop_pos - line);
                nsCString value(prop_pos + 2);
                if (key.EqualsIgnoreCase("upgrade") &&
                    value.EqualsIgnoreCase("websocket")) {
                    isWebSocket = true;
                } else if (key.EqualsIgnoreCase("sec-websocket-version")) {
                    version = value;
                } else if (key.EqualsIgnoreCase("sec-websocket-key")) {
                    wsKey = value;
                } else if (key.EqualsIgnoreCase("sec-websocket-protocol")) {
                    protocol = value;
                }
            }
        }

        if (!isWebSocket) {
            return false;
        }

        if (!(version.EqualsLiteral("7") ||
              version.EqualsLiteral("8") ||
              version.EqualsLiteral("13"))) {
            return false;
        }

        if (!(protocol.EqualsIgnoreCase("binary"))) {
            return false;
        }

        
        nsAutoCString guid("258EAFA5-E914-47DA-95CA-C5AB0DC85B11");
        nsAutoCString res;
        SHA1Sum sha1;
        nsCString combined(wsKey + guid);
        sha1.update(combined.get(), combined.Length());
        uint8_t digest[SHA1Sum::kHashSize]; 
        sha1.finish(digest);
        nsCString newString(reinterpret_cast<char*>(digest), SHA1Sum::kHashSize);
        Base64Encode(newString, res);

        nsCString response("HTTP/1.1 101 Switching Protocols\r\n");
        response.AppendLiteral("Upgrade: websocket\r\n");
        response.AppendLiteral("Connection: Upgrade\r\n");
        response.Append(nsCString("Sec-WebSocket-Accept: ") + res + nsCString("\r\n"));
        response.AppendLiteral("Sec-WebSocket-Protocol: binary\r\n\r\n");
        uint32_t written = 0;
        uint32_t size = response.Length();
        while (written < size) {
            uint32_t cnt;
            rv = mOutputStream->Write(const_cast<char*>(response.get()) + written,
                                     size - written, &cnt);
            if (NS_FAILED(rv))
                return false;

            written += cnt;
        }
        mOutputStream->Flush();

        return true;
    }

    nsCOMPtr<nsIOutputStream> mOutputStream;
    nsCOMPtr<nsIAsyncInputStream> mInputStream;
    nsCOMPtr<nsISocketTransport> mTransport;
    SocketStateType mState;
};

NS_IMPL_ISUPPORTS(LayerScopeWebSocketHandler, nsIInputStreamCallback);

class LayerScopeWebSocketManager {
public:
    LayerScopeWebSocketManager();
    ~LayerScopeWebSocketManager();

    void AddConnection(nsISocketTransport *aTransport)
    {
        MOZ_ASSERT(aTransport);
        nsRefPtr<LayerScopeWebSocketHandler> temp = new LayerScopeWebSocketHandler();
        temp->OpenStream(aTransport);
        mHandlers.AppendElement(temp.get());
    }

    void RemoveConnection(uint32_t aIndex)
    {
        MOZ_ASSERT(aIndex < mHandlers.Length());
        mHandlers.RemoveElementAt(aIndex);
    }

    void RemoveAllConnections()
    {
        mHandlers.Clear();
    }

    bool WriteAll(void *ptr, uint32_t size)
    {
        for (int32_t i = mHandlers.Length() - 1; i >= 0; --i) {
            if (!mHandlers[i]->WriteToStream(ptr, size)) {
                
                RemoveConnection(i);
            }
        }

        return true;
    }

    bool IsConnected()
    {
        return (mHandlers.Length() != 0) ? true : false;
    }

    void AppendDebugData(DebugGLData *aDebugData);
    void CleanDebugData();
    void DispatchDebugData();
private:
    nsTArray<nsRefPtr<LayerScopeWebSocketHandler> > mHandlers;
    nsCOMPtr<nsIThread> mDebugSenderThread;
    nsRefPtr<DebugDataSender> mCurrentSender;
    nsCOMPtr<nsIServerSocket> mServerSocket;
};


class WebSocketHelper
{
public:
    static void CreateServerSocket()
    {
        
        NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");
        if (!sWebSocketManager) {
            sWebSocketManager = new LayerScopeWebSocketManager();
        }
    }

    static void DestroyServerSocket()
    {
        
        if (sWebSocketManager) {
            sWebSocketManager->RemoveAllConnections();
        }
    }

    static LayerScopeWebSocketManager* GetSocketManager()
    {
        return sWebSocketManager;
    }

private:
    static StaticAutoPtr<LayerScopeWebSocketManager> sWebSocketManager;
};

StaticAutoPtr<LayerScopeWebSocketManager> WebSocketHelper::sWebSocketManager;







class DebugGLData: public LinkedListElement<DebugGLData> {
public:
    DebugGLData(Packet::DataType aDataType)
        : mDataType(aDataType)
    { }

    virtual ~DebugGLData() { }

    Packet::DataType GetDataType() const { return mDataType; }

    virtual bool Write() = 0;

    static bool WriteToStream(Packet& aPacket) {
        if (!WebSocketHelper::GetSocketManager())
            return true;

        uint32_t size = aPacket.ByteSize();
        auto data = MakeUnique<uint8_t[]>(size);
        aPacket.SerializeToArray(data.get(), size);
        return WebSocketHelper::GetSocketManager()->WriteAll(data.get(), size);
    }

protected:
    Packet::DataType mDataType;
};

class DebugGLFrameStatusData : public DebugGLData
{
public:
    DebugGLFrameStatusData(Packet::DataType aDataType,
                           int64_t aValue)
        : DebugGLData(aDataType),
          mFrameStamp(aValue)
    { }

    DebugGLFrameStatusData(Packet::DataType aDataType)
        : DebugGLData(aDataType),
          mFrameStamp(0)
    { }

    int64_t GetFrameStamp() const { return mFrameStamp; }

    virtual bool Write() MOZ_OVERRIDE {
        Packet packet;
        packet.set_type(mDataType);

        FramePacket* fp = packet.mutable_frame();
        fp->set_value(static_cast<uint64_t>(mFrameStamp));

        if (!WriteToStream(packet))
            return false;
        return true;
    }

protected:
    int64_t mFrameStamp;
};

class DebugGLTextureData : public DebugGLData {
public:
    DebugGLTextureData(GLContext* cx,
                       void* layerRef,
                       GLenum target,
                       GLuint name,
                       DataSourceSurface* img)
        : DebugGLData(Packet::TEXTURE),
          mLayerRef(layerRef),
          mTarget(target),
          mName(name),
          mContextAddress(reinterpret_cast<intptr_t>(cx)),
          mDatasize(0)
    {
        
        
        
        
        pack(img);
    }

    const void* GetLayerRef() const { return mLayerRef; }
    GLuint GetName() const { return mName; }
    GLenum GetTextureTarget() const { return mTarget; }
    intptr_t GetContextAddress() const { return mContextAddress; }
    uint32_t GetDataSize() const { return mDatasize; }

    virtual bool Write() MOZ_OVERRIDE {
        if (!WriteToStream(mPacket))
            return false;
        return true;
    }

private:
    void pack(DataSourceSurface* aImage) {
        mPacket.set_type(mDataType);

        TexturePacket* tp = mPacket.mutable_texture();
        tp->set_layerref(reinterpret_cast<uint64_t>(mLayerRef));
        tp->set_name(mName);
        tp->set_target(mTarget);
        tp->set_dataformat(LOCAL_GL_RGBA);
        tp->set_glcontext(static_cast<uint64_t>(mContextAddress));

        if (aImage) {
            tp->set_width(aImage->GetSize().width);
            tp->set_height(aImage->GetSize().height);
            tp->set_stride(aImage->Stride());

            mDatasize = aImage->GetSize().height * aImage->Stride();

            auto compresseddata = MakeUnique<char[]>(LZ4::maxCompressedSize(mDatasize));
            if (compresseddata) {
                int ndatasize = LZ4::compress((char*)aImage->GetData(),
                                              mDatasize,
                                              compresseddata.get());
                if (ndatasize > 0) {
                    mDatasize = ndatasize;
                    tp->set_dataformat((1 << 16 | tp->dataformat()));
                    tp->set_data(compresseddata.get(), mDatasize);
                } else {
                    NS_WARNING("Compress data failed");
                    tp->set_data(aImage->GetData(), mDatasize);
                }
            } else {
                NS_WARNING("Couldn't new compressed data.");
                tp->set_data(aImage->GetData(), mDatasize);
            }
        } else {
            tp->set_width(0);
            tp->set_height(0);
            tp->set_stride(0);
        }
    }

protected:
    void* mLayerRef;
    GLenum mTarget;
    GLuint mName;
    intptr_t mContextAddress;
    uint32_t mDatasize;

    
    Packet mPacket;
};

class DebugGLColorData : public DebugGLData {
public:
    DebugGLColorData(void* layerRef,
                     const gfxRGBA& color,
                     int width,
                     int height)
        : DebugGLData(Packet::COLOR),
          mLayerRef(layerRef),
          mColor(color.Packed()),
          mSize(width, height)
    { }

    const void* GetLayerRef() const { return mLayerRef; }
    uint32_t GetColor() const { return mColor; }
    const nsIntSize& GetSize() const { return mSize; }

    virtual bool Write() MOZ_OVERRIDE {
        Packet packet;
        packet.set_type(mDataType);

        ColorPacket* cp = packet.mutable_color();
        cp->set_layerref(reinterpret_cast<uint64_t>(mLayerRef));
        cp->set_color(mColor);
        cp->set_width(mSize.width);
        cp->set_height(mSize.height);

        if (!WriteToStream(packet))
            return false;
        return true;
    }

protected:
    void* mLayerRef;
    uint32_t mColor;
    nsIntSize mSize;
};

class DebugGLLayersData : public DebugGLData {
public:
    DebugGLLayersData(UniquePtr<Packet> aPacket)
        : DebugGLData(Packet::LAYERS),
          mPacket(Move(aPacket))
    { }

    virtual bool Write() MOZ_OVERRIDE {
        mPacket->set_type(mDataType);

        if (!WriteToStream(*mPacket))
            return false;
        return true;
    }

protected:
    UniquePtr<Packet> mPacket;
};

class DebugListener : public nsIServerSocketListener
{
    virtual ~DebugListener() { }

public:

    NS_DECL_THREADSAFE_ISUPPORTS

    DebugListener() { }

    

    NS_IMETHODIMP OnSocketAccepted(nsIServerSocket *aServ,
                                   nsISocketTransport *aTransport)
    {
        if (!WebSocketHelper::GetSocketManager())
            return NS_OK;

        printf_stderr("*** LayerScope: Accepted connection\n");
        WebSocketHelper::GetSocketManager()->AddConnection(aTransport);
        return NS_OK;
    }

    NS_IMETHODIMP OnStopListening(nsIServerSocket *aServ,
                                  nsresult aStatus)
    {
        return NS_OK;
    }
};

NS_IMPL_ISUPPORTS(DebugListener, nsIServerSocketListener);


class DebugDataSender : public nsIRunnable
{
    virtual ~DebugDataSender() {
        Cleanup();
    }

public:

    NS_DECL_THREADSAFE_ISUPPORTS

    DebugDataSender() { }

    void Append(DebugGLData *d) {
        mList.insertBack(d);
    }

    void Cleanup() {
        if (mList.isEmpty())
            return;

        DebugGLData *d;
        while ((d = mList.popFirst()) != nullptr)
            delete d;
    }

    

    NS_IMETHODIMP Run() {
        DebugGLData *d;
        nsresult rv = NS_OK;

        while ((d = mList.popFirst()) != nullptr) {
            UniquePtr<DebugGLData> cleaner(d);
            if (!d->Write()) {
                rv = NS_ERROR_FAILURE;
                break;
            }
        }

        Cleanup();

        if (NS_FAILED(rv)) {
            WebSocketHelper::DestroyServerSocket();
        }

        return NS_OK;
    }

protected:
    LinkedList<DebugGLData> mList;
};

NS_IMPL_ISUPPORTS(DebugDataSender, nsIRunnable);











class SenderHelper
{

public:
    static void SendLayer(LayerComposite* aLayer,
                          int aWidth,
                          int aHeight);

    static void SendEffectChain(gl::GLContext* aGLContext,
                                const EffectChain& aEffectChain,
                                int aWidth = 0,
                                int aHeight = 0);


private:
    static void SendColor(void* aLayerRef,
                          const gfxRGBA& aColor,
                          int aWidth,
                          int aHeight);
    static void SendTextureSource(GLContext* aGLContext,
                                  void* aLayerRef,
                                  TextureSourceOGL* aSource,
                                  bool aFlipY);
    static void SendTexturedEffect(GLContext* aGLContext,
                                   void* aLayerRef,
                                   const TexturedEffect* aEffect);
    static void SendYCbCrEffect(GLContext* aGLContext,
                                void* aLayerRef,
                                const EffectYCbCr* aEffect);
};





void
SenderHelper::SendLayer(LayerComposite* aLayer,
                        int aWidth,
                        int aHeight)
{
    MOZ_ASSERT(aLayer && aLayer->GetLayer());
    if (!aLayer || !aLayer->GetLayer()) {
        return;
    }

    switch (aLayer->GetLayer()->GetType()) {
        case Layer::TYPE_COLOR: {
            EffectChain effect;
            aLayer->GenEffectChain(effect);
            SenderHelper::SendEffectChain(nullptr, effect, aWidth, aHeight);
            break;
        }
        case Layer::TYPE_IMAGE:
        case Layer::TYPE_CANVAS:
        case Layer::TYPE_THEBES: {
            
            CompositableHost* compHost = aLayer->GetCompositableHost();
            Compositor* comp = compHost->GetCompositor();
            
            if (LayersBackend::LAYERS_OPENGL == comp->GetBackendType()) {
                CompositorOGL* compOGL = static_cast<CompositorOGL*>(comp);
                EffectChain effect;
                
                AutoLockCompositableHost lock(compHost);
                aLayer->GenEffectChain(effect);
                SenderHelper::SendEffectChain(compOGL->gl(), effect);
            }
            break;
        }
        case Layer::TYPE_CONTAINER:
        default:
            break;
    }
}

void
SenderHelper::SendColor(void* aLayerRef,
                        const gfxRGBA& aColor,
                        int aWidth,
                        int aHeight)
{
    WebSocketHelper::GetSocketManager()->AppendDebugData(
        new DebugGLColorData(aLayerRef, aColor, aWidth, aHeight));
}

void
SenderHelper::SendTextureSource(GLContext* aGLContext,
                                void* aLayerRef,
                                TextureSourceOGL* aSource,
                                bool aFlipY)
{
    MOZ_ASSERT(aGLContext);
    if (!aGLContext) {
        return;
    }

    GLenum textureTarget = aSource->GetTextureTarget();
    ShaderConfigOGL config = ShaderConfigFromTargetAndFormat(textureTarget,
                                                             aSource->GetFormat());
    int shaderConfig = config.mFeatures;

    aSource->BindTexture(LOCAL_GL_TEXTURE0, gfx::Filter::LINEAR);

    GLuint textureId = 0;
    
    
    if (textureTarget == LOCAL_GL_TEXTURE_2D) {
        aGLContext->GetUIntegerv(LOCAL_GL_TEXTURE_BINDING_2D, &textureId);
    } else if (textureTarget == LOCAL_GL_TEXTURE_EXTERNAL) {
        aGLContext->GetUIntegerv(LOCAL_GL_TEXTURE_BINDING_EXTERNAL, &textureId);
    } else if (textureTarget == LOCAL_GL_TEXTURE_RECTANGLE) {
        aGLContext->GetUIntegerv(LOCAL_GL_TEXTURE_BINDING_RECTANGLE, &textureId);
    }

    gfx::IntSize size = aSource->GetSize();

    
    
    RefPtr<DataSourceSurface> img =
        aGLContext->ReadTexImageHelper()->ReadTexImage(0, textureTarget,
                                                       size,
                                                       shaderConfig, aFlipY);

    WebSocketHelper::GetSocketManager()->AppendDebugData(
        new DebugGLTextureData(aGLContext, aLayerRef, textureTarget,
                               textureId, img));
}

void
SenderHelper::SendTexturedEffect(GLContext* aGLContext,
                                 void* aLayerRef,
                                 const TexturedEffect* aEffect)
{
    TextureSourceOGL* source = aEffect->mTexture->AsSourceOGL();
    if (!source)
        return;

    bool flipY = false;
    SendTextureSource(aGLContext, aLayerRef, source, flipY);
}

void
SenderHelper::SendYCbCrEffect(GLContext* aGLContext,
                              void* aLayerRef,
                              const EffectYCbCr* aEffect)
{
    TextureSource* sourceYCbCr = aEffect->mTexture;
    if (!sourceYCbCr)
        return;

    const int Y = 0, Cb = 1, Cr = 2;
    TextureSourceOGL* sourceY =  sourceYCbCr->GetSubSource(Y)->AsSourceOGL();
    TextureSourceOGL* sourceCb = sourceYCbCr->GetSubSource(Cb)->AsSourceOGL();
    TextureSourceOGL* sourceCr = sourceYCbCr->GetSubSource(Cr)->AsSourceOGL();

    bool flipY = false;
    SendTextureSource(aGLContext, aLayerRef, sourceY,  flipY);
    SendTextureSource(aGLContext, aLayerRef, sourceCb, flipY);
    SendTextureSource(aGLContext, aLayerRef, sourceCr, flipY);
}

void
SenderHelper::SendEffectChain(GLContext* aGLContext,
                              const EffectChain& aEffectChain,
                              int aWidth,
                              int aHeight)
{
    const Effect* primaryEffect = aEffectChain.mPrimaryEffect;
    switch (primaryEffect->mType) {
        case EffectTypes::RGB: {
            const TexturedEffect* texturedEffect =
                static_cast<const TexturedEffect*>(primaryEffect);
            SendTexturedEffect(aGLContext, aEffectChain.mLayerRef, texturedEffect);
            break;
        }
        case EffectTypes::YCBCR: {
            const EffectYCbCr* yCbCrEffect =
                static_cast<const EffectYCbCr*>(primaryEffect);
            SendYCbCrEffect(aGLContext, aEffectChain.mLayerRef, yCbCrEffect);
            break;
        }
        case EffectTypes::SOLID_COLOR: {
            const EffectSolidColor* solidColorEffect =
                static_cast<const EffectSolidColor*>(primaryEffect);
            gfxRGBA color(solidColorEffect->mColor.r,
                          solidColorEffect->mColor.g,
                          solidColorEffect->mColor.b,
                          solidColorEffect->mColor.a);
            SendColor(aEffectChain.mLayerRef, color, aWidth, aHeight);
            break;
        }
        case EffectTypes::COMPONENT_ALPHA:
        case EffectTypes::RENDER_TARGET:
        default:
            break;
    }

    
    
}




LayerScopeWebSocketManager::LayerScopeWebSocketManager()
{
    NS_NewThread(getter_AddRefs(mDebugSenderThread));

    mServerSocket = do_CreateInstance(NS_SERVERSOCKET_CONTRACTID);
    int port = gfxPrefs::LayerScopePort();
    mServerSocket->Init(port, false, -1);
    mServerSocket->AsyncListen(new DebugListener);
}

LayerScopeWebSocketManager::~LayerScopeWebSocketManager()
{
}

void
LayerScopeWebSocketManager::AppendDebugData(DebugGLData *aDebugData)
{
    if (!mCurrentSender) {
        mCurrentSender = new DebugDataSender();
    }

    mCurrentSender->Append(aDebugData);
}

void
LayerScopeWebSocketManager::CleanDebugData()
{
    if (mCurrentSender) {
        mCurrentSender->Cleanup();
    }
}

void
LayerScopeWebSocketManager::DispatchDebugData()
{
    mDebugSenderThread->Dispatch(mCurrentSender, NS_DISPATCH_NORMAL);
    mCurrentSender = nullptr;
}





void
LayerScope::Init()
{
    if (!gfxPrefs::LayerScopeEnabled()) {
        return;
    }

    
    WebSocketHelper::CreateServerSocket();
}

void
LayerScope::DeInit()
{
    
    WebSocketHelper::DestroyServerSocket();
}

void
LayerScope::SendEffectChain(gl::GLContext* aGLContext,
                            const EffectChain& aEffectChain,
                            int aWidth,
                            int aHeight)
{
    
    if (!CheckSendable()) {
        return;
    }
    SenderHelper::SendEffectChain(aGLContext, aEffectChain, aWidth, aHeight);
}

void
LayerScope::SendLayer(LayerComposite* aLayer,
                      int aWidth,
                      int aHeight)
{
    
    if (!CheckSendable()) {
        return;
    }
    SenderHelper::SendLayer(aLayer, aWidth, aHeight);
}

void
LayerScope::SendLayerDump(UniquePtr<Packet> aPacket)
{
    
    if (!CheckSendable()) {
        return;
    }
    WebSocketHelper::GetSocketManager()->AppendDebugData(
        new DebugGLLayersData(Move(aPacket)));
}

bool
LayerScope::CheckSendable()
{
    if (!WebSocketHelper::GetSocketManager()) {
        return false;
    }
    if (!WebSocketHelper::GetSocketManager()->IsConnected()) {
        return false;
    }
    return true;
}

void
LayerScope::CleanLayer()
{
    if (CheckSendable()) {
        WebSocketHelper::GetSocketManager()->CleanDebugData();
    }
}




LayerScopeAutoFrame::LayerScopeAutoFrame(int64_t aFrameStamp)
{
    
    BeginFrame(aFrameStamp);
}

LayerScopeAutoFrame::~LayerScopeAutoFrame()
{
    
    EndFrame();
}

void
LayerScopeAutoFrame::BeginFrame(int64_t aFrameStamp)
{
    if (!LayerScope::CheckSendable()) {
        return;
    }

    WebSocketHelper::GetSocketManager()->AppendDebugData(
        new DebugGLFrameStatusData(Packet::FRAMESTART, aFrameStamp));
}

void
LayerScopeAutoFrame::EndFrame()
{
    if (!LayerScope::CheckSendable()) {
        return;
    }

    WebSocketHelper::GetSocketManager()->AppendDebugData(
        new DebugGLFrameStatusData(Packet::FRAMEEND));
    WebSocketHelper::GetSocketManager()->DispatchDebugData();
}

} 
} 
