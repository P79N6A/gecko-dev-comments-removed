





#ifndef SkMatrixClipStateMgr_DEFINED
#define SkMatrixClipStateMgr_DEFINED

#include "SkCanvas.h"
#include "SkMatrix.h"
#include "SkRegion.h"
#include "SkRRect.h"
#include "SkTypes.h"
#include "SkTDArray.h"

class SkPictureRecord;
class SkWriter32;
































class SkMatrixClipStateMgr {
public:
    static const int32_t kIdentityWideOpenStateID = 0;
    static const int kIdentityMatID = 0;

    class MatrixClipState : SkNoncopyable {
    public:
        class MatrixInfo {
        public:
            void reset() {
                fMatrixID = kIdentityMatID;
                fMatrix.reset();
            }

            void preTranslate(SkScalar dx, SkScalar dy) {
                fMatrixID = -1;
                fMatrix.preTranslate(dx, dy);
            }

            void preScale(SkScalar sx, SkScalar sy) {
                fMatrixID = -1;
                fMatrix.preScale(sx, sy);
            }

            void preRotate(SkScalar degrees) {
                fMatrixID = -1;
                fMatrix.preRotate(degrees);
            }

            void preSkew(SkScalar sx, SkScalar sy) {
                fMatrixID = -1;
                fMatrix.preSkew(sx, sy);
            }

            void preConcat(const SkMatrix& matrix) {
                fMatrixID = -1;
                fMatrix.preConcat(matrix);
            }

            void setMatrix(const SkMatrix& matrix) {
                fMatrixID = -1;
                fMatrix = matrix;
            }

            int getID(SkMatrixClipStateMgr* mgr) {
                if (fMatrixID >= 0) {
                    return fMatrixID;
                }

                fMatrixID = mgr->addMatToDict(fMatrix);
                return fMatrixID;
            }

        private:
            SkMatrix fMatrix;
            int      fMatrixID;

            typedef SkNoncopyable INHERITED;
        };

        class ClipInfo : SkNoncopyable {
        public:
            ClipInfo() {}

            bool clipRect(const SkRect& rect,
                          SkRegion::Op op,
                          bool doAA,
                          int matrixID) {
                ClipOp* newClip = fClips.append();
                newClip->fClipType = kRect_ClipType;
                newClip->fGeom.fRRect.setRect(rect);   
                newClip->fOp = op;
                newClip->fDoAA = doAA;
                newClip->fMatrixID = matrixID;
                return false;
            }

            bool clipRRect(const SkRRect& rrect,
                           SkRegion::Op op,
                           bool doAA,
                           int matrixID) {
                ClipOp* newClip = fClips.append();
                newClip->fClipType = kRRect_ClipType;
                newClip->fGeom.fRRect = rrect;
                newClip->fOp = op;
                newClip->fDoAA = doAA;
                newClip->fMatrixID = matrixID;
                return false;
            }

            bool clipPath(SkPictureRecord* picRecord,
                          const SkPath& path,
                          SkRegion::Op op,
                          bool doAA,
                          int matrixID);
            bool clipRegion(SkPictureRecord* picRecord,
                            int regionID,
                            SkRegion::Op op,
                            int matrixID);
            void writeClip(int* curMatID, SkMatrixClipStateMgr* mgr);

            SkDEBUGCODE(int numClips() const { return fClips.count(); })

        private:
            enum ClipType {
                kRect_ClipType,
                kRRect_ClipType,
                kPath_ClipType,
                kRegion_ClipType
            };

            class ClipOp {
            public:
                ClipType     fClipType;

                union {
                    SkRRect fRRect;        
                    int     fPathID;
                    int     fRegionID;
                } fGeom;

                bool         fDoAA;
                SkRegion::Op fOp;

                
                int          fMatrixID;
            };

            SkTDArray<ClipOp> fClips;

            typedef SkNoncopyable INHERITED;
        };

        MatrixClipState(MatrixClipState* prev)
            : fPrev(prev)
        {
            fHasOpen = false;

            if (NULL == prev) {
                fLayerID = 0;

                fMatrixInfoStorage.reset();
                fMatrixInfo = &fMatrixInfoStorage;
                fClipInfo = &fClipInfoStorage;  

                
                fMCStateID = kIdentityWideOpenStateID;
#ifdef SK_DEBUG
                fExpectedDepth = 1;
#endif
            }
            else {
                fLayerID = prev->fLayerID;

                fMatrixInfoStorage = *prev->fMatrixInfo;
                fMatrixInfo = &fMatrixInfoStorage;

                
                fClipInfo = &fClipInfoStorage;

                
                
                fMCStateID = prev->fMCStateID;
#ifdef SK_DEBUG
                fExpectedDepth = prev->fExpectedDepth;
#endif
            }

            fIsSaveLayer = false;
        }

        MatrixInfo*  fMatrixInfo;
        MatrixInfo   fMatrixInfoStorage;

        ClipInfo*    fClipInfo;
        ClipInfo     fClipInfoStorage;

        
        int          fLayerID;
        
        bool         fIsSaveLayer;

        
        SkTDArray<int>* fSavedSkipOffsets;

        
        bool         fHasOpen;

        MatrixClipState* fPrev;

#ifdef SK_DEBUG
        int              fExpectedDepth;    
#endif

        int32_t     fMCStateID;
    };

    enum CallType {
        kMatrix_CallType,
        kClip_CallType,
        kOther_CallType
    };

    SkMatrixClipStateMgr();
    ~SkMatrixClipStateMgr();

    void init(SkPictureRecord* picRecord) {
        
        
        fPicRecord = picRecord;
    }

    SkPictureRecord* getPicRecord() { return fPicRecord; }

    
    
    
    int getSaveCount() const { return fMatrixClipStack.count(); }

    int save();

    int saveLayer(const SkRect* bounds, const SkPaint* paint, SkCanvas::SaveFlags flags);

    bool isDrawingToLayer() const {
        return fCurMCState->fLayerID > 0;
    }

    void restore();

    void translate(SkScalar dx, SkScalar dy) {
        this->call(kMatrix_CallType);
        fCurMCState->fMatrixInfo->preTranslate(dx, dy);
    }

    void scale(SkScalar sx, SkScalar sy) {
        this->call(kMatrix_CallType);
        fCurMCState->fMatrixInfo->preScale(sx, sy);
    }

    void rotate(SkScalar degrees) {
        this->call(kMatrix_CallType);
        fCurMCState->fMatrixInfo->preRotate(degrees);
    }

    void skew(SkScalar sx, SkScalar sy) {
        this->call(kMatrix_CallType);
        fCurMCState->fMatrixInfo->preSkew(sx, sy);
    }

    void concat(const SkMatrix& matrix) {
        this->call(kMatrix_CallType);
        fCurMCState->fMatrixInfo->preConcat(matrix);
    }

    void setMatrix(const SkMatrix& matrix) {
        this->call(kMatrix_CallType);
        fCurMCState->fMatrixInfo->setMatrix(matrix);
    }

    bool clipRect(const SkRect& rect, SkRegion::Op op, bool doAA) {
        this->call(SkMatrixClipStateMgr::kClip_CallType);
        return fCurMCState->fClipInfo->clipRect(rect, op, doAA,
                                                fCurMCState->fMatrixInfo->getID(this));
    }

    bool clipRRect(const SkRRect& rrect, SkRegion::Op op, bool doAA) {
        this->call(SkMatrixClipStateMgr::kClip_CallType);
        return fCurMCState->fClipInfo->clipRRect(rrect, op, doAA,
                                                 fCurMCState->fMatrixInfo->getID(this));
    }

    bool clipPath(const SkPath& path, SkRegion::Op op, bool doAA) {
        this->call(SkMatrixClipStateMgr::kClip_CallType);
        return fCurMCState->fClipInfo->clipPath(fPicRecord, path, op, doAA,
                                                fCurMCState->fMatrixInfo->getID(this));
    }

    bool clipRegion(const SkRegion& region, SkRegion::Op op) {
        this->call(SkMatrixClipStateMgr::kClip_CallType);
        int regionID = this->addRegionToDict(region);
        return fCurMCState->fClipInfo->clipRegion(fPicRecord, regionID, op,
                                                  fCurMCState->fMatrixInfo->getID(this));
    }

    bool call(CallType callType);

    void fillInSkips(SkWriter32* writer, int32_t restoreOffset);

    void finish();

protected:
    SkPictureRecord* fPicRecord;

    uint32_t         fMatrixClipStackStorage[43]; 
    SkDeque          fMatrixClipStack;
    MatrixClipState* fCurMCState;

    
    
    
    
    
    SkTDArray<SkMatrix> fMatrixDict;

    SkTDArray<SkRegion*> fRegionDict;

    
    int32_t          fCurOpenStateID;
    
    
    
    
    SkTDArray<int32_t>  *fSkipOffsets;  

    SkDEBUGCODE(void validate();)

    int MCStackPush();

    void addClipOffset(size_t offset) {
        SkASSERT(NULL != fSkipOffsets);
        SkASSERT(kIdentityWideOpenStateID != fCurOpenStateID);
        SkASSERT(fCurMCState->fHasOpen);
        SkASSERT(!fCurMCState->fIsSaveLayer);

        *fSkipOffsets->append() = SkToS32(offset);
    }

    void writeDeltaMat(int currentMatID, int desiredMatID);
    static int32_t   NewMCStateID();

    int addRegionToDict(const SkRegion& region);
    const SkRegion* lookupRegion(int index) {
        SkASSERT(index >= 0 && index < fRegionDict.count());
        return fRegionDict[index];
    }

    
    
    int addMatToDict(const SkMatrix& mat);
    const SkMatrix& lookupMat(int index) {
        SkASSERT(index >= 0 && index < fMatrixDict.count());
        return fMatrixDict[index];
    }

    bool isNestingMCState(int stateID);

#ifdef SK_DEBUG
    int fActualDepth;
#endif

    
    
    SkTDArray<int> fStateIDStack;
};

#endif
