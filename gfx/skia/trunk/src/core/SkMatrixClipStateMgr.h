





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

    class MatrixClipState {
    public:
        class MatrixInfo {
        public:
            void reset() {
                fMatrixID = kIdentityMatID;
                fMatrix.reset();
            }

            bool preTranslate(SkScalar dx, SkScalar dy) {
                fMatrixID = -1;
                return fMatrix.preTranslate(dx, dy);
            }

            bool preScale(SkScalar sx, SkScalar sy) {
                fMatrixID = -1;
                return fMatrix.preScale(sx, sy);
            }

            bool preRotate(SkScalar degrees) {
                fMatrixID = -1;
                return fMatrix.preRotate(degrees);
            }

            bool preSkew(SkScalar sx, SkScalar sy) {
                fMatrixID = -1;
                return fMatrix.preSkew(sx, sy);
            }

            bool preConcat(const SkMatrix& matrix) {
                fMatrixID = -1;
                return fMatrix.preConcat(matrix);
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
        };

        class ClipInfo : public SkNoncopyable {
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
                newClip->fOffset = kInvalidJumpOffset;
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
                newClip->fOffset = kInvalidJumpOffset;
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
            void writeClip(int* curMatID,
                           SkMatrixClipStateMgr* mgr,
                           bool* overrideFirstOp);
            void fillInSkips(SkWriter32* writer, int32_t restoreOffset);

#ifdef SK_DEBUG
            void checkOffsetNotEqual(int32_t offset) {
                for (int i = 0; i < fClips.count(); ++i) {
                    ClipOp& curClip = fClips[i];
                    SkASSERT(offset != curClip.fOffset);
                }
            }
#endif
        private:
            enum ClipType {
                kRect_ClipType,
                kRRect_ClipType,
                kPath_ClipType,
                kRegion_ClipType
            };

            static const int kInvalidJumpOffset = -1;

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

                
                
                int32_t      fOffset;
            };

            SkTDArray<ClipOp> fClips;

            typedef SkNoncopyable INHERITED;
        };

        MatrixClipState(MatrixClipState* prev, int flags)
#ifdef SK_DEBUG
            : fPrev(prev)
#endif
        {
            if (NULL == prev) {
                fLayerID = 0;

                fMatrixInfoStorage.reset();
                fMatrixInfo = &fMatrixInfoStorage;
                fClipInfo = &fClipInfoStorage;  

                
                fMCStateID = kIdentityWideOpenStateID;
            }
            else {
                fLayerID = prev->fLayerID;

                if (flags & SkCanvas::kMatrix_SaveFlag) {
                    fMatrixInfoStorage = *prev->fMatrixInfo;
                    fMatrixInfo = &fMatrixInfoStorage;
                } else {
                    fMatrixInfo = prev->fMatrixInfo;
                }

                if (flags & SkCanvas::kClip_SaveFlag) {
                    
                    fClipInfo = &fClipInfoStorage;
                } else {
                    fClipInfo = prev->fClipInfo;
                }

                
                
                fMCStateID = prev->fMCStateID;
            }

            fIsSaveLayer = false;
        }

        MatrixInfo*  fMatrixInfo;
        MatrixInfo   fMatrixInfoStorage;

        ClipInfo*    fClipInfo;
        ClipInfo     fClipInfoStorage;

        
        int          fLayerID;
        
        bool         fIsSaveLayer;

        
        int32_t      fSaveLayerBaseStateID;
        bool         fSaveLayerBracketed;

#ifdef SK_DEBUG
        MatrixClipState* fPrev; 
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

    int save(SkCanvas::SaveFlags flags);

    int saveLayer(const SkRect* bounds, const SkPaint* paint, SkCanvas::SaveFlags flags);

    bool isDrawingToLayer() const {
        return fCurMCState->fLayerID > 0;
    }

    void restore();

    bool translate(SkScalar dx, SkScalar dy) {
        this->call(kMatrix_CallType);
        return fCurMCState->fMatrixInfo->preTranslate(dx, dy);
    }

    bool scale(SkScalar sx, SkScalar sy) {
        this->call(kMatrix_CallType);
        return fCurMCState->fMatrixInfo->preScale(sx, sy);
    }

    bool rotate(SkScalar degrees) {
        this->call(kMatrix_CallType);
        return fCurMCState->fMatrixInfo->preRotate(degrees);
    }

    bool skew(SkScalar sx, SkScalar sy) {
        this->call(kMatrix_CallType);
        return fCurMCState->fMatrixInfo->preSkew(sx, sy);
    }

    bool concat(const SkMatrix& matrix) {
        this->call(kMatrix_CallType);
        return fCurMCState->fMatrixInfo->preConcat(matrix);
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

    void fillInSkips(SkWriter32* writer, int32_t restoreOffset) {
        
        
        SkDeque::F2BIter iter(fMatrixClipStack);

        for (const MatrixClipState* state = (const MatrixClipState*) iter.next();
             state != NULL;
             state = (const MatrixClipState*) iter.next()) {
            state->fClipInfo->fillInSkips(writer, restoreOffset);
        }
    }

    void finish();

protected:
    SkPictureRecord* fPicRecord;

    uint32_t         fMatrixClipStackStorage[43]; 
    SkDeque          fMatrixClipStack;
    MatrixClipState* fCurMCState;

    
    
    
    
    
    SkTDArray<SkMatrix> fMatrixDict;

    SkTDArray<SkRegion*> fRegionDict;

    
    int32_t          fCurOpenStateID;

    SkDEBUGCODE(void validate();)

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
};

#endif
