






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
                                                                SkMatrixClipStateMgr* mgr) {
    for (int i = 0; i < fClips.count(); ++i) {
        ClipOp& curClip = fClips[i];

        
        
        
        
        
        
        
        
        
        
        mgr->writeDeltaMat(*curMatID, curClip.fMatrixID);
        *curMatID = curClip.fMatrixID;

        size_t offset = 0;

        switch (curClip.fClipType) {
        case kRect_ClipType:
            offset = mgr->getPicRecord()->recordClipRect(curClip.fGeom.fRRect.rect(),
                                                         curClip.fOp, curClip.fDoAA);
            break;
        case kRRect_ClipType:
            offset = mgr->getPicRecord()->recordClipRRect(curClip.fGeom.fRRect, curClip.fOp,
                                                         curClip.fDoAA);
            break;
        case kPath_ClipType:
            offset = mgr->getPicRecord()->recordClipPath(curClip.fGeom.fPathID, curClip.fOp,
                                                         curClip.fDoAA);
            break;
        case kRegion_ClipType: {
            const SkRegion* region = mgr->lookupRegion(curClip.fGeom.fRegionID);
            offset = mgr->getPicRecord()->recordClipRegion(*region, curClip.fOp);
            break;
        }
        default:
            SkASSERT(0);
        }

        mgr->addClipOffset(offset);
    }
}

SkMatrixClipStateMgr::SkMatrixClipStateMgr()
    : fPicRecord(NULL)
    , fMatrixClipStack(sizeof(MatrixClipState),
                       fMatrixClipStackStorage,
                       sizeof(fMatrixClipStackStorage))
    , fCurOpenStateID(kIdentityWideOpenStateID) {

    fSkipOffsets = SkNEW(SkTDArray<int>);

    
    fMatrixDict.append()->reset();

    fCurMCState = (MatrixClipState*)fMatrixClipStack.push_back();
    new (fCurMCState) MatrixClipState(NULL);    

#ifdef SK_DEBUG
    fActualDepth = 0;
#endif
}

SkMatrixClipStateMgr::~SkMatrixClipStateMgr() {
    for (int i = 0; i < fRegionDict.count(); ++i) {
        SkDELETE(fRegionDict[i]);
    }

    SkDELETE(fSkipOffsets);
}


int SkMatrixClipStateMgr::MCStackPush() {
    MatrixClipState* newTop = (MatrixClipState*)fMatrixClipStack.push_back();
    new (newTop) MatrixClipState(fCurMCState); 
    fCurMCState = newTop;

    SkDEBUGCODE(this->validate();)

    return fMatrixClipStack.count();
}

int SkMatrixClipStateMgr::save() {
    SkDEBUGCODE(this->validate();)

    return this->MCStackPush();
}

int SkMatrixClipStateMgr::saveLayer(const SkRect* bounds, const SkPaint* paint,
                                    SkCanvas::SaveFlags flags) {
#ifdef SK_DEBUG
    if (fCurMCState->fIsSaveLayer) {
        SkASSERT(0 == fSkipOffsets->count());
    }
#endif

    
    
    SkDEBUGCODE(bool saved =) this->call(kOther_CallType);

    int result = this->MCStackPush();
    ++fCurMCState->fLayerID;
    fCurMCState->fIsSaveLayer = true;

#ifdef SK_DEBUG
    if (saved) {
        fCurMCState->fExpectedDepth++; 
    }
    fCurMCState->fExpectedDepth++;   
#endif

    *fStateIDStack.append() = fCurOpenStateID;
    fCurMCState->fSavedSkipOffsets = fSkipOffsets;

    
    
    fSkipOffsets = SkNEW(SkTDArray<int>);

    fPicRecord->recordSaveLayer(bounds, paint, flags);
#ifdef SK_DEBUG
    fActualDepth++;
#endif
    return result;
}

void SkMatrixClipStateMgr::restore() {
    SkDEBUGCODE(this->validate();)

    if (fCurMCState->fIsSaveLayer) {
        if (fCurMCState->fHasOpen) {
            fCurMCState->fHasOpen = false;
            fPicRecord->recordRestore(); 
#ifdef SK_DEBUG
            SkASSERT(fActualDepth > 0);
            fActualDepth--;
#endif
        } else {
            SkASSERT(0 == fSkipOffsets->count());
        }

        
        
        
        fPicRecord->recordRestore(false); 
#ifdef SK_DEBUG
        SkASSERT(fActualDepth > 0);
        fActualDepth--;
#endif

        SkASSERT(fStateIDStack.count() >= 1);
        fCurOpenStateID = fStateIDStack[fStateIDStack.count()-1];
        fStateIDStack.pop();

        SkASSERT(0 == fSkipOffsets->count());
        SkASSERT(NULL != fCurMCState->fSavedSkipOffsets);

        SkDELETE(fSkipOffsets);
        fSkipOffsets = fCurMCState->fSavedSkipOffsets;
    }

    bool prevHadOpen = fCurMCState->fHasOpen;
    bool prevWasSaveLayer = fCurMCState->fIsSaveLayer;

    fCurMCState->~MatrixClipState();       
    fMatrixClipStack.pop_back();
    fCurMCState = (MatrixClipState*)fMatrixClipStack.back();

    if (!prevWasSaveLayer) {
        fCurMCState->fHasOpen = prevHadOpen;
    }

    if (fCurMCState->fIsSaveLayer) {
        if (0 != fSkipOffsets->count()) {
            SkASSERT(fCurMCState->fHasOpen);
        }
    }

    SkDEBUGCODE(this->validate();)
}


int32_t SkMatrixClipStateMgr::NewMCStateID() {
    
    
    static int32_t gMCStateID = kIdentityWideOpenStateID;
    ++gMCStateID;
    return gMCStateID;
}

bool SkMatrixClipStateMgr::isNestingMCState(int stateID) {
    return fStateIDStack.count() > 0 && fStateIDStack[fStateIDStack.count()-1] == fCurOpenStateID;
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

    if (kIdentityWideOpenStateID != fCurOpenStateID &&
        !this->isNestingMCState(fCurOpenStateID)) {
        
        
        fPicRecord->recordRestore();    
        fCurMCState->fHasOpen = false;
#ifdef SK_DEBUG
        SkASSERT(fActualDepth > 0);
        fActualDepth--;
#endif
    }

    
    fCurOpenStateID = fCurMCState->fMCStateID;

    if (kIdentityWideOpenStateID == fCurOpenStateID) {
        SkASSERT(0 == fActualDepth);
        SkASSERT(!fCurMCState->fHasOpen);
        SkASSERT(0 == fSkipOffsets->count());
        return false;
    }

    SkASSERT(!fCurMCState->fHasOpen);
    SkASSERT(0 == fSkipOffsets->count());
    fCurMCState->fHasOpen = true;
    fPicRecord->recordSave();
#ifdef SK_DEBUG
    fActualDepth++;
    SkASSERT(fActualDepth == fCurMCState->fExpectedDepth);
#endif

    
    SkDeque::Iter iter(fMatrixClipStack, SkDeque::Iter::kBack_IterStart);
    const MatrixClipState* state;
    
    
    for (state = (const MatrixClipState*) iter.prev();
         state != NULL;
         state = (const MatrixClipState*) iter.prev()) {
        if (state->fIsSaveLayer) {
            break;
        }
    }

    int curMatID;

    if (NULL == state) {
        
        iter.reset(fMatrixClipStack, SkDeque::Iter::kFront_IterStart);
        state = (const MatrixClipState*) iter.next();
        curMatID = kIdentityMatID;
    } else {
        
        
        iter.next();
        SkDEBUGCODE(const MatrixClipState* test = (const MatrixClipState*)) iter.next();
        SkASSERT(test == state);

        curMatID = state->fMatrixInfo->getID(this);

        
        
        
        
        if (NULL != state->fPrev && state->fClipInfo == state->fPrev->fClipInfo) {
            
            
            state = (const MatrixClipState*) iter.next();
        }
    }

    for ( ; state != NULL; state = (const MatrixClipState*) iter.next()) {
         state->fClipInfo->writeClip(&curMatID, this);
    }

    
    
    
    
    
    if (kIdentityMatID != fCurMCState->fMatrixInfo->getID(this)) {
        
        
        
        this->writeDeltaMat(curMatID, fCurMCState->fMatrixInfo->getID(this));
    }

    SkDEBUGCODE(this->validate();)
    return true;
}


void SkMatrixClipStateMgr::fillInSkips(SkWriter32* writer, int32_t restoreOffset) {
    for (int i = 0; i < fSkipOffsets->count(); ++i) {
        SkDEBUGCODE(int32_t peek = writer->readTAt<int32_t>((*fSkipOffsets)[i]);)
        SkASSERT(-1 == peek);
        writer->overwriteTAt<int32_t>((*fSkipOffsets)[i], restoreOffset);
    }

    fSkipOffsets->rewind();
    SkASSERT(0 == fSkipOffsets->count());
}

void SkMatrixClipStateMgr::finish() {
    if (kIdentityWideOpenStateID != fCurOpenStateID) {
        fPicRecord->recordRestore();    
        fCurMCState->fHasOpen = false;
#ifdef SK_DEBUG
        SkASSERT(fActualDepth > 0);
        fActualDepth--;
#endif
        fCurOpenStateID = kIdentityWideOpenStateID;
        SkASSERT(!fCurMCState->fHasOpen);
    }
}

#ifdef SK_DEBUG
void SkMatrixClipStateMgr::validate() {
    if (fCurOpenStateID == fCurMCState->fMCStateID && !this->isNestingMCState(fCurOpenStateID)) {
        
        
        SkDeque::Iter iter(fMatrixClipStack, SkDeque::Iter::kBack_IterStart);
        int clipCount = 0;
        for (const MatrixClipState* state = (const MatrixClipState*) iter.prev();
             state != NULL;
             state = (const MatrixClipState*) iter.prev()) {
            if (NULL == state->fPrev || state->fPrev->fClipInfo != state->fClipInfo) {
                clipCount += state->fClipInfo->numClips();
            }
            if (state->fIsSaveLayer) {
                break;
            }
        }

        SkASSERT(fSkipOffsets->count() == clipCount);
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
