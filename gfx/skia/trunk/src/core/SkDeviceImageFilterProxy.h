






#ifndef SkDeviceImageFilterProxy_DEFINED
#define SkDeviceImageFilterProxy_DEFINED

#include "SkImageFilter.h"

class SkDeviceImageFilterProxy : public SkImageFilter::Proxy {
public:
    SkDeviceImageFilterProxy(SkBaseDevice* device) : fDevice(device) {}

    virtual SkBaseDevice* createDevice(int w, int h) SK_OVERRIDE {
        return fDevice->createCompatibleDevice(SkBitmap::kARGB_8888_Config,
                                               w, h, false);
    }
    virtual bool canHandleImageFilter(const SkImageFilter* filter) SK_OVERRIDE {
        return fDevice->canHandleImageFilter(filter);
    }
    virtual bool filterImage(const SkImageFilter* filter, const SkBitmap& src,
                             const SkMatrix& ctm,
                             SkBitmap* result, SkIPoint* offset) SK_OVERRIDE {
        return fDevice->filterImage(filter, src, ctm, result, offset);
    }

private:
    SkBaseDevice* fDevice;
};

#endif
