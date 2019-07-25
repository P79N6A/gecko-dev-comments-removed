















#ifndef ANDROID_IOMX_H_

#define ANDROID_IOMX_H_

#include <binder/IInterface.h>
#include <ui/GraphicBuffer.h>
#include <utils/List.h>
#include <utils/String8.h>

#include <OMX_Core.h>
#include <OMX_Video.h>

#include "jni.h"

namespace android {

class IMemory;
class IOMXObserver;
class IOMXRenderer;
class Surface;

class IOMX : public IInterface {
public:
    DECLARE_META_INTERFACE(OMX);

    typedef void *buffer_id;
    typedef void *node_id;

    
    
    
    virtual bool livesLocally(pid_t pid) = 0;

    struct ComponentInfo {
        String8 mName;
        List<String8> mRoles;
    };
    virtual status_t listNodes(List<ComponentInfo> *list) = 0;

    virtual status_t allocateNode(
            const char *name, const sp<IOMXObserver> &observer,
            node_id *node) = 0;

    virtual status_t freeNode(node_id node) = 0;

    virtual status_t sendCommand(
            node_id node, OMX_COMMANDTYPE cmd, OMX_S32 param) = 0;

    virtual status_t getParameter(
            node_id node, OMX_INDEXTYPE index,
            void *params, size_t size) = 0;

    virtual status_t setParameter(
            node_id node, OMX_INDEXTYPE index,
            const void *params, size_t size) = 0;

    virtual status_t getConfig(
            node_id node, OMX_INDEXTYPE index,
            void *params, size_t size) = 0;

    virtual status_t setConfig(
            node_id node, OMX_INDEXTYPE index,
            const void *params, size_t size) = 0;

    virtual status_t getState(
            node_id node, OMX_STATETYPE* state) = 0;

    virtual status_t storeMetaDataInBuffers(
            node_id node, OMX_U32 port_index, OMX_BOOL enable) = 0;

    virtual status_t enableGraphicBuffers(
            node_id node, OMX_U32 port_index, OMX_BOOL enable) = 0;

    virtual status_t getGraphicBufferUsage(
            node_id node, OMX_U32 port_index, OMX_U32* usage) = 0;

    virtual status_t useBuffer(
            node_id node, OMX_U32 port_index, const sp<IMemory> &params,
            buffer_id *buffer) = 0;

    virtual status_t useGraphicBuffer(
            node_id node, OMX_U32 port_index,
            const sp<GraphicBuffer> &graphicBuffer, buffer_id *buffer) = 0;

    
    
    
    
    virtual status_t allocateBuffer(
            node_id node, OMX_U32 port_index, size_t size,
            buffer_id *buffer, void **buffer_data) = 0;

    virtual status_t allocateBufferWithBackup(
            node_id node, OMX_U32 port_index, const sp<IMemory> &params,
            buffer_id *buffer) = 0;

    virtual status_t freeBuffer(
            node_id node, OMX_U32 port_index, buffer_id buffer) = 0;

    virtual status_t fillBuffer(node_id node, buffer_id buffer) = 0;

    virtual status_t emptyBuffer(
            node_id node,
            buffer_id buffer,
            OMX_U32 range_offset, OMX_U32 range_length,
            OMX_U32 flags, OMX_TICKS timestamp) = 0;

    virtual status_t getExtensionIndex(
            node_id node,
            const char *parameter_name,
            OMX_INDEXTYPE *index) = 0;
};

struct omx_message {
    enum {
        EVENT,
        EMPTY_BUFFER_DONE,
        FILL_BUFFER_DONE,

    } type;

    IOMX::node_id node;

    union {
        
        struct {
            OMX_EVENTTYPE event;
            OMX_U32 data1;
            OMX_U32 data2;
        } event_data;

        
        struct {
            IOMX::buffer_id buffer;
        } buffer_data;

        
        struct {
            IOMX::buffer_id buffer;
            OMX_U32 range_offset;
            OMX_U32 range_length;
            OMX_U32 flags;
            OMX_TICKS timestamp;
            OMX_PTR platform_private;
            OMX_PTR data_ptr;
        } extended_buffer_data;

    } u;
};

class IOMXObserver : public IInterface {
public:
    DECLARE_META_INTERFACE(OMXObserver);

    virtual void onMessage(const omx_message &msg) = 0;
};



class BnOMX : public BnInterface<IOMX> {
public:
    virtual status_t onTransact(
            uint32_t code, const Parcel &data, Parcel *reply,
            uint32_t flags = 0);
};

class BnOMXObserver : public BnInterface<IOMXObserver> {
public:
    virtual status_t onTransact(
            uint32_t code, const Parcel &data, Parcel *reply,
            uint32_t flags = 0);
};

struct CodecProfileLevel {
    OMX_U32 mProfile;
    OMX_U32 mLevel;
};

}  

#endif  
