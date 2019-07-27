






#include "SkImageGenerator.h"

#ifndef SK_SUPPORT_LEGACY_IMAGEGENERATORAPI
bool SkImageGenerator::getInfo(SkImageInfo* info) {
    SkImageInfo dummy;
    if (NULL == info) {
        info = &dummy;
    }
    return this->onGetInfo(info);
}

bool SkImageGenerator::getPixels(const SkImageInfo& info, void* pixels, size_t rowBytes,
                                 SkPMColor ctable[], int* ctableCount) {
    if (kUnknown_SkColorType == info.colorType()) {
        return false;
    }
    if (NULL == pixels) {
        return false;
    }
    if (rowBytes < info.minRowBytes()) {
        return false;
    }

    if (kIndex_8_SkColorType == info.colorType()) {
        if (NULL == ctable || NULL == ctableCount) {
            return false;
        }
    } else {
        if (ctableCount) {
            *ctableCount = 0;
        }
        ctableCount = NULL;
        ctable = NULL;
    }

    bool success = this->onGetPixels(info, pixels, rowBytes, ctable, ctableCount);

    if (success && ctableCount) {
        SkASSERT(*ctableCount >= 0 && *ctableCount <= 256);
    }
    return success;
}

bool SkImageGenerator::getPixels(const SkImageInfo& info, void* pixels, size_t rowBytes) {
    SkASSERT(kIndex_8_SkColorType != info.colorType());
    if (kIndex_8_SkColorType == info.colorType()) {
        return false;
    }
    return this->getPixels(info, pixels, rowBytes, NULL, NULL);
}
#endif

bool SkImageGenerator::getYUV8Planes(SkISize sizes[3], void* planes[3], size_t rowBytes[3]) {
#ifdef SK_DEBUG
    
    SkASSERT(NULL != sizes);

    bool isValidWithPlanes = (NULL != planes) && (NULL != rowBytes) &&
        ((NULL != planes[0]) && (NULL != planes[1]) && (NULL != planes[2]) &&
         (0  != rowBytes[0]) && (0  != rowBytes[1]) && (0  != rowBytes[2]));
    bool isValidWithoutPlanes =
        ((NULL == planes) ||
         ((NULL == planes[0]) && (NULL == planes[1]) && (NULL == planes[2]))) &&
        ((NULL == rowBytes) ||
         ((0 == rowBytes[0]) && (0 == rowBytes[1]) && (0 == rowBytes[2])));

    
    
    SkASSERT(isValidWithPlanes || isValidWithoutPlanes);

    
    
    SkASSERT(!isValidWithPlanes ||
             ((sizes[0].fWidth  >= 0) &&
              (sizes[0].fHeight >= 0) &&
              (sizes[1].fWidth  >= 0) &&
              (sizes[1].fHeight >= 0) &&
              (sizes[2].fWidth  >= 0) &&
              (sizes[2].fHeight >= 0) &&
              (rowBytes[0] >= (size_t)sizes[0].fWidth) &&
              (rowBytes[1] >= (size_t)sizes[1].fWidth) &&
              (rowBytes[2] >= (size_t)sizes[2].fWidth)));
#endif

    return this->onGetYUV8Planes(sizes, planes, rowBytes);
}

bool SkImageGenerator::onGetYUV8Planes(SkISize sizes[3], void* planes[3], size_t rowBytes[3]) {
    return false;
}



SkData* SkImageGenerator::onRefEncodedData() {
    return NULL;
}

bool SkImageGenerator::onGetInfo(SkImageInfo*) {
    return false;
}

bool SkImageGenerator::onGetPixels(const SkImageInfo&, void*, size_t, SkPMColor*, int*) {
    return false;
}
