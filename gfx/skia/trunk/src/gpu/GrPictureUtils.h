






#ifndef GrPictureUtils_DEFINED
#define GrPictureUtils_DEFINED

#include "SkPicture.h"
#include "SkTDArray.h"



class GPUAccelData : public SkPicture::AccelData {
public:
    
    struct SaveLayerInfo {
        
        
        
        bool fValid;
        
        SkISize fSize;
        
        
        SkMatrix fCTM;
        
        
        SkIPoint fOffset;
        
        
        const SkPaint* fPaint;
        
        size_t  fSaveLayerOpID;
        
        size_t  fRestoreOpID;
        
        
        bool    fHasNestedLayers;
        
        bool    fIsNested;
    };

    GPUAccelData(Key key) : INHERITED(key) { }

    virtual ~GPUAccelData() {
        for (int i = 0; i < fSaveLayerInfo.count(); ++i) {
            SkDELETE(fSaveLayerInfo[i].fPaint);
        }
    }

    void addSaveLayerInfo(const SaveLayerInfo& info) {
        SkASSERT(info.fSaveLayerOpID < info.fRestoreOpID);
        *fSaveLayerInfo.push() = info;
    }

    int numSaveLayers() const { return fSaveLayerInfo.count(); }

    const SaveLayerInfo& saveLayerInfo(int index) const {
        SkASSERT(index < fSaveLayerInfo.count());

        return fSaveLayerInfo[index];
    }

    
    
    static SkPicture::AccelData::Key ComputeAccelDataKey();

private:
    SkTDArray<SaveLayerInfo> fSaveLayerInfo;

    typedef SkPicture::AccelData INHERITED;
};

void GatherGPUInfo(const SkPicture* pict, GPUAccelData* accelData);

#endif 
