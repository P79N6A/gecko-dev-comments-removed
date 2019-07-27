







#include "SkCanvas.h"
#include "SkPicture.h"
#include "SkPictureData.h"
#include "SkPictureReplacementPlayback.h"


SkPictureReplacementPlayback::PlaybackReplacements::ReplacementInfo*
SkPictureReplacementPlayback::PlaybackReplacements::push() {
    SkDEBUGCODE(this->validate());
    return fReplacements.push();
}

void SkPictureReplacementPlayback::PlaybackReplacements::freeAll() {
    for (int i = 0; i < fReplacements.count(); ++i) {
        SkDELETE(fReplacements[i].fBM);
    }
    fReplacements.reset();
}

#ifdef SK_DEBUG
void SkPictureReplacementPlayback::PlaybackReplacements::validate() const {
    
    if (fReplacements.count() > 0) {
        SkASSERT(fReplacements[0].fStart < fReplacements[0].fStop);

        for (int i = 1; i < fReplacements.count(); ++i) {
            SkASSERT(fReplacements[i].fStart < fReplacements[i].fStop);
            SkASSERT(fReplacements[i - 1].fStop < fReplacements[i].fStart);
        }
    }
}
#endif


SkPictureReplacementPlayback::PlaybackReplacements::ReplacementInfo*
SkPictureReplacementPlayback::PlaybackReplacements::lookupByStart(size_t start) {
    SkDEBUGCODE(this->validate());
    for (int i = 0; i < fReplacements.count(); ++i) {
        if (start == fReplacements[i].fStart) {
            return &fReplacements[i];
        } else if (start < fReplacements[i].fStart) {
            return NULL;  
        }
    }

    return NULL;
}

bool SkPictureReplacementPlayback::replaceOps(SkPictureStateTree::Iterator* iter,
                                              SkReader32* reader,
                                              SkCanvas* canvas,
                                              const SkMatrix& initialMatrix) {
    if (NULL != fReplacements) {
        
        PlaybackReplacements::ReplacementInfo* temp =
                                  fReplacements->lookupByStart(reader->offset());
        if (NULL != temp) {
            SkASSERT(NULL != temp->fBM);
            SkASSERT(NULL != temp->fPaint);
            canvas->save();
            canvas->setMatrix(initialMatrix);
            SkRect src = SkRect::Make(temp->fSrcRect);
            SkRect dst = SkRect::MakeXYWH(temp->fPos.fX, temp->fPos.fY,
                                          temp->fSrcRect.width(),
                                          temp->fSrcRect.height());
            canvas->drawBitmapRectToRect(*temp->fBM, &src, dst, temp->fPaint);
            canvas->restore();

            if (iter->isValid()) {
                
                
                canvas->save();

                
                
                
                
                
                

                uint32_t skipTo;
                do {
                    skipTo = iter->nextDraw();
                    if (SkPictureStateTree::Iterator::kDrawComplete == skipTo) {
                        break;
                    }

                    if (skipTo <= temp->fStop) {
                        reader->setOffset(skipTo);
                        uint32_t size;
                        DrawType op = ReadOpAndSize(reader, &size);
                        
                        
                        
                        
                        if (SAVE_LAYER == op) {
                            canvas->save();
                        }
                    }
                } while (skipTo <= temp->fStop);

                if (SkPictureStateTree::Iterator::kDrawComplete == skipTo) {
                    reader->setOffset(reader->size());      
                    return true;
                }

                reader->setOffset(skipTo);
            } else {
                reader->setOffset(temp->fStop);
                uint32_t size;
                SkDEBUGCODE(DrawType op = ) ReadOpAndSize(reader, &size);
                SkASSERT(RESTORE == op);
            }

            return true;
        }
    }

    return false;
}

void SkPictureReplacementPlayback::draw(SkCanvas* canvas, SkDrawPictureCallback* callback) {
    AutoResetOpID aroi(this);
    SkASSERT(0 == fCurOffset);

    SkPictureStateTree::Iterator it;

    if (!this->initIterator(&it, canvas, fActiveOpsList)) {
        return;  
    }

    SkReader32 reader(fPictureData->opData()->bytes(), fPictureData->opData()->size());

    StepIterator(&it, &reader);

    
    SkMatrix initialMatrix = canvas->getTotalMatrix();

    SkAutoCanvasRestore acr(canvas, false);

    while (!reader.eof()) {
        if (NULL != callback && callback->abortDrawing()) {
            return;
        }

        if (this->replaceOps(&it, &reader, canvas, initialMatrix)) {
            continue;
        }

        fCurOffset = reader.offset();
        uint32_t size;
        DrawType op = ReadOpAndSize(&reader, &size);
        if (NOOP == op) {
            
            SkipIterTo(&it, &reader, fCurOffset + size);
            continue;
        }

        this->handleOp(&reader, op, size, canvas, initialMatrix);

        StepIterator(&it, &reader);
    }
}
