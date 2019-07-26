
















#include <stdint.h>
#include <sys/types.h>
#include <errno.h>
#include <cutils/log.h>

#include <gui/IDisplayEventConnection.h>
#include <gui/GraphicBufferAlloc.h>

#include "FakeSurfaceComposer.h"

namespace android {


void FakeSurfaceComposer::instantiate() {
    defaultServiceManager()->addService(
            String16("SurfaceFlinger"), new FakeSurfaceComposer());
}

FakeSurfaceComposer::FakeSurfaceComposer()
    : BnSurfaceComposer()
{
}

FakeSurfaceComposer::~FakeSurfaceComposer()
{
}

sp<ISurfaceComposerClient> FakeSurfaceComposer::createConnection()
{
    return nullptr;
}

sp<IGraphicBufferAlloc> FakeSurfaceComposer::createGraphicBufferAlloc()
{
    sp<GraphicBufferAlloc> gba(new GraphicBufferAlloc());
    return gba;
}

sp<IBinder> FakeSurfaceComposer::createDisplay(const String8& displayName,
        bool secure)
{
    return nullptr;
}

sp<IBinder> FakeSurfaceComposer::getBuiltInDisplay(int32_t id) {
    return nullptr;
}

void FakeSurfaceComposer::setTransactionState(
        const Vector<ComposerState>& state,
        const Vector<DisplayState>& displays,
        uint32_t flags)
{
}

void FakeSurfaceComposer::bootFinished()
{
}

bool FakeSurfaceComposer::authenticateSurfaceTexture(
        const sp<IGraphicBufferProducer>& bufferProducer) const {
    return false;
}

sp<IDisplayEventConnection> FakeSurfaceComposer::createDisplayEventConnection() {
    return nullptr;
}

status_t FakeSurfaceComposer::captureScreen(const sp<IBinder>& display,
        const sp<IGraphicBufferProducer>& producer,
        uint32_t reqWidth, uint32_t reqHeight,
        uint32_t minLayerZ, uint32_t maxLayerZ,
        bool isCpuConsumer) {
    return INVALID_OPERATION;
}

void FakeSurfaceComposer::blank(const sp<IBinder>& display) {
}

void FakeSurfaceComposer::unblank(const sp<IBinder>& display) {
}

status_t FakeSurfaceComposer::getDisplayInfo(const sp<IBinder>& display, DisplayInfo* info) {
    return INVALID_OPERATION;
}

}; 
