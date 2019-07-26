






#include "SkMatrixClipStateMgr.h"
#include "SkPictureRecord.h"

bool SkMatrixClipStateMgr::MatrixClipState::ClipInfo::clipPath(SkPictureRecord* picRecord,
                                                               const SkPath& path,
                                                               SkRegion::Op op,
                                                               bool doAA,
                                                               int matrixID) {
    int pathID = picRecord->addPathToHeap(path);

    ClipOp* newClip = fClips.append();
    newClip->fClipType = kPath_ClipType;
    newClip->fGeom.fPathID = pathID;
    newClip->fOp = op;
    newClip->fDoAA = doAA;
    newClip->fMatrixID = matrixID;
    newClip->fOffset = kInvalidJumpOffset;
    return false;
}

bool SkMatrixClipStateMgr::MatrixClipState::ClipInfo::clipRegion(SkPictureRecord* picRecord,
                                                                 int regionID,
                                                                 SkRegion::Op op,
                                                                 int matrixID) {
    
    ClipOp* newClip = fClips.append();
    newClip->fClipType = kRegion_ClipType;
    newClip->fGeom.fRegionID = regionID;
    newClip->fOp = op;
    newClip->fDoAA = true;      
    newClip->fMatrixID = matrixID;
    newClip->fOffset = kInvalidJumpOffset;
    return false;
}

void SkMatrixClipStateMgr::writeDeltaMat(int currentMatID, int desiredMatID) {
    const SkMatrix& current = this->lookupMat(currentMatID);
    const SkMatrix& desired = this->lookupMat(desiredMatID);

    SkMatrix delta;
    bool result = current.invert(&delta);
    if (result) {
        delta.preConcat(desired);
    }
    fPicRecord->recordConcat(delta);
}



void SkMatrixClipStateMgr::MatrixClipState::ClipInfo::writeClip(int* curMatID,
                                                                SkMatrixClipStateMgr* mgr,
                                                                bool* overrideFirstOp) {
    for (int i = 0; i < fClips.count(); ++i) {
        ClipOp& curClip = fClips[i];

        SkRegion::Op op = curClip.fOp;
        if (*overrideFirstOp) {
            op = SkRegion::kReplace_Op;
            *overrideFirstOp = false;
        }

        
        
        
        
        
        
        
        
        
        
        mgr->writeDeltaMat(*curMatID, curClip.fMatrixID);
        *curMatID = curClip.fMatrixID;

        switch (curClip.fClipType) {
        case kRect_ClipType:
            curClip.fOffset = mgr->getPicRecord()->recordClipRect(curClip.fGeom.fRRect.rect(),
                                                                  op, curClip.fDoAA);
            break;
        case kRRect_ClipType:
            curClip.fOffset = mgr->getPicRecord()->recordClipRRect(curClip.fGeom.fRRect, op,
                                                                   curClip.fDoAA);
            break;
        case kPath_ClipType:
            curClip.fOffset = mgr->getPicRecord()->recordClipPath(curClip.fGeom.fPathID, op,
                                                                  curClip.fDoAA);
            break;
        case kRegion_ClipType: {
            const SkRegion* region = mgr->lookupRegion(curClip.fGeom.fRegionID);
            curClip.fOffset = mgr->getPicRecord()->recordClipRegion(*region, op);
            break;
        }
        default:
            SkASSERT(0);
        }
    }
}


void SkMatrixClipStateMgr::MatrixClipState::ClipInfo::fillInSkips(SkWriter32* writer,
                                                                  int32_t restoreOffset) {
    for (int i = 0; i < fClips.count(); ++i) {
        ClipOp& curClip = fClips[i];

        if (-1 == curClip.fOffset) {
            continue;
        }


        writer->overwriteTAt(curClip.fOffset, restoreOffset);
        SkDEBUGCODE(curClip.fOffset = -1;)
    }
}

SkMatrixClipStateMgr::SkMatrixClipStateMgr()
    : fPicRecord(NULL)
    , fMatrixClipStack(sizeof(MatrixClipState),
                       fMatrixClipStackStorage,
                       sizeof(fMatrixClipStackStorage))
    , fCurOpenStateID(kIdentityWideOpenStateID) {

    
    fMatrixDict.append()->reset();

    fCurMCState = (MatrixClipState*)fMatrixClipStack.push_back();
    new (fCurMCState) MatrixClipState(NULL, 0);    
}

SkMatrixClipStateMgr::~SkMatrixClipStateMgr() {
    for (int i = 0; i < fRegionDict.count(); ++i) {
        SkDELETE(fRegionDict[i]);
    }
}


int SkMatrixClipStateMgr::save(SkCanvas::SaveFlags flags) {
    SkDEBUGCODE(this->validate();)

    MatrixClipState* newTop = (MatrixClipState*)fMatrixClipStack.push_back();
    new (newTop) MatrixClipState(fCurMCState, flags); 
    fCurMCState = newTop;

    SkDEBUGCODE(this->validate();)

    return fMatrixClipStack.count();
}

int SkMatrixClipStateMgr::saveLayer(const SkRect* bounds, const SkPaint* paint,
                                    SkCanvas::SaveFlags flags) {
    int result = this->save(flags);
    ++fCurMCState->fLayerID;
    fCurMCState->fIsSaveLayer = true;

    fCurMCState->fSaveLayerBracketed = this->call(kOther_CallType);
    fCurMCState->fSaveLayerBaseStateID = fCurOpenStateID;
    fPicRecord->recordSaveLayer(bounds, paint,
                                (SkCanvas::SaveFlags)(flags| SkCanvas::kMatrixClip_SaveFlag));
    return result;
}

void SkMatrixClipStateMgr::restore() {
    SkDEBUGCODE(this->validate();)

    if (fCurMCState->fIsSaveLayer) {
        if (fCurMCState->fSaveLayerBaseStateID != fCurOpenStateID) {
            fPicRecord->recordRestore(); 
        }
        
        
        
        fPicRecord->recordRestore(false); 

        
        
        if (fCurMCState->fSaveLayerBracketed) {
            fPicRecord->recordRestore(false);
        }

        
        fCurOpenStateID = kIdentityWideOpenStateID;
    }

    fCurMCState->~MatrixClipState();       
    fMatrixClipStack.pop_back();
    fCurMCState = (MatrixClipState*)fMatrixClipStack.back();

    SkDEBUGCODE(this->validate();)
}


int32_t SkMatrixClipStateMgr::NewMCStateID() {
    
    
    static int32_t gMCStateID = kIdentityWideOpenStateID;
    ++gMCStateID;
    return gMCStateID;
}

bool SkMatrixClipStateMgr::call(CallType callType) {
    SkDEBUGCODE(this->validate();)

    if (kMatrix_CallType == callType || kClip_CallType == callType) {
        fCurMCState->fMCStateID = NewMCStateID();
        SkDEBUGCODE(this->validate();)
        return false;
    }

    SkASSERT(kOther_CallType == callType);

    if (fCurMCState->fMCStateID == fCurOpenStateID) {
        
        SkDEBUGCODE(this->validate();)
        return false;
    }

    if (kIdentityWideOpenStateID != fCurOpenStateID) {
        fPicRecord->recordRestore();    
    }

    
    fCurOpenStateID = fCurMCState->fMCStateID;

    fPicRecord->recordSave(SkCanvas::kMatrixClip_SaveFlag);

    
    SkDeque::F2BIter iter(fMatrixClipStack);
    bool firstClip = true;

    int curMatID = kIdentityMatID;
    for (const MatrixClipState* state = (const MatrixClipState*) iter.next();
         state != NULL;
         state = (const MatrixClipState*) iter.next()) {
         state->fClipInfo->writeClip(&curMatID, this, &firstClip);
    }

    
    if (kIdentityMatID != fCurMCState->fMatrixInfo->getID(this)) {
        
        
        
        this->writeDeltaMat(curMatID, fCurMCState->fMatrixInfo->getID(this));
    }

    SkDEBUGCODE(this->validate();)

    return true;
}

void SkMatrixClipStateMgr::finish() {
    if (kIdentityWideOpenStateID != fCurOpenStateID) {
        fPicRecord->recordRestore();    
        fCurOpenStateID = kIdentityWideOpenStateID;
    }
}

#ifdef SK_DEBUG
void SkMatrixClipStateMgr::validate() {
    if (fCurOpenStateID == fCurMCState->fMCStateID) {
        
        
        SkDeque::F2BIter iter(fMatrixClipStack);

        for (const MatrixClipState* state = (const MatrixClipState*) iter.next();
             state != NULL;
             state = (const MatrixClipState*) iter.next()) {
            state->fClipInfo->checkOffsetNotEqual(-1);
        }
    }
}
#endif

int SkMatrixClipStateMgr::addRegionToDict(const SkRegion& region) {
    int index = fRegionDict.count();
    *fRegionDict.append() = SkNEW(SkRegion(region));
    return index;
}

int SkMatrixClipStateMgr::addMatToDict(const SkMatrix& mat) {
    if (mat.isIdentity()) {
        return kIdentityMatID;
    }

    *fMatrixDict.append() = mat;
    return fMatrixDict.count()-1;
}
