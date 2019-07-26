







#ifndef SkPathRef_DEFINED
#define SkPathRef_DEFINED

#include "SkRefCnt.h"
#include <stddef.h> 





#define NEW_PICTURE_FORMAT 0















class SkPathRef : public ::SkRefCnt {
public:
    SK_DECLARE_INST_COUNT(SkPathRef);

    class Editor {
    public:
        Editor(SkAutoTUnref<SkPathRef>* pathRef,
               int incReserveVerbs = 0,
               int incReservePoints = 0) {
            if (pathRef->get()->getRefCnt() > 1) {
                SkPathRef* copy = SkNEW(SkPathRef);
                copy->copy(*pathRef->get(), incReserveVerbs, incReservePoints);
                pathRef->reset(copy);
            } else {
                (*pathRef)->incReserve(incReserveVerbs, incReservePoints);
            }
            fPathRef = pathRef->get();
            fPathRef->fGenerationID = 0;
            SkDEBUGCODE(sk_atomic_inc(&fPathRef->fEditorsAttached);)
        }

        ~Editor() { SkDEBUGCODE(sk_atomic_dec(&fPathRef->fEditorsAttached);) }

        


        SkPoint* points() { return fPathRef->fPoints; }

        


        SkPoint* atPoint(int i) {
            SkASSERT((unsigned) i < (unsigned) fPathRef->fPointCnt);
            return this->points() + i;
        };

        



        SkPoint* growForVerb(SkPath::Verb verb) {
            fPathRef->validate();
            return fPathRef->growForVerb(verb);
        }

        




        void grow(int newVerbs, int newPts, uint8_t** verbs, SkPoint** pts) {
            SkASSERT(NULL != verbs);
            SkASSERT(NULL != pts);
            fPathRef->validate();
            int oldVerbCnt = fPathRef->fVerbCnt;
            int oldPointCnt = fPathRef->fPointCnt;
            SkASSERT(verbs && pts);
            fPathRef->grow(newVerbs, newPts);
            *verbs = fPathRef->fVerbs - oldVerbCnt;
            *pts = fPathRef->fPoints + oldPointCnt;
            fPathRef->validate();
        }

        



        void resetToSize(int newVerbCnt, int newPointCnt) {
            fPathRef->resetToSize(newVerbCnt, newPointCnt);
        }
        


        SkPathRef* pathRef() { return fPathRef; }

    private:
        SkPathRef* fPathRef;
    };

public:

    


    static SkPathRef* CreateEmpty() {
        static SkAutoTUnref<SkPathRef> gEmptyPathRef(SkNEW(SkPathRef));
        gEmptyPathRef.get()->ref();
        return gEmptyPathRef.get();
    }

    


    static void CreateTransformedCopy(SkAutoTUnref<SkPathRef>* dst,
                                      const SkPathRef& src,
                                      const SkMatrix& matrix) {
        src.validate();
        if (matrix.isIdentity()) {
            if (dst->get() != &src) {
                dst->reset(const_cast<SkPathRef*>(&src));
                (*dst)->validate();
                src.ref();
            }
            return;
        }
        int32_t rcnt = dst->get()->getRefCnt();
        if (&src == dst->get() && 1 == rcnt) {
            matrix.mapPoints((*dst)->fPoints, (*dst)->fPointCnt);
            return;
        } else if (rcnt > 1) {
            dst->reset(SkNEW(SkPathRef));
        }
        (*dst)->resetToSize(src.fVerbCnt, src.fPointCnt);
        memcpy((*dst)->verbsMemWritable(), src.verbsMemBegin(), src.fVerbCnt * sizeof(uint8_t));
        matrix.mapPoints((*dst)->fPoints, src.points(), src.fPointCnt);
        (*dst)->validate();
    }

#if NEW_PICTURE_FORMAT
    static SkPathRef* CreateFromBuffer(SkRBuffer* buffer) {
        SkPathRef* ref = SkNEW(SkPathRef);
        ref->fGenerationID = buffer->readU32();
        int32_t verbCount = buffer->readS32();
        int32_t pointCount = buffer->readS32();
        ref->resetToSize(verbCount, pointCount);

        SkASSERT(verbCount == ref->countVerbs());
        SkASSERT(pointCount == ref->countPoints());
        buffer->read(ref->verbsMemWritable(), verbCount * sizeof(uint8_t));
        buffer->read(ref->fPoints, pointCount * sizeof(SkPoint));
        return ref;
    }
#else
    static SkPathRef* CreateFromBuffer(int verbCount, int pointCount, SkRBuffer* buffer) {
        SkPathRef* ref = SkNEW(SkPathRef);

        ref->resetToSize(verbCount, pointCount);
        SkASSERT(verbCount == ref->countVerbs());
        SkASSERT(pointCount == ref->countPoints());
        buffer->read(ref->fPoints, pointCount * sizeof(SkPoint));
        for (int i = 0; i < verbCount; ++i) {
            ref->fVerbs[~i] = buffer->readU8();
        }
        return ref;
    }
#endif

    




    static void Rewind(SkAutoTUnref<SkPathRef>* pathRef) {
        if (1 == (*pathRef)->getRefCnt()) {
            (*pathRef)->validate();
            (*pathRef)->fVerbCnt = 0;
            (*pathRef)->fPointCnt = 0;
            (*pathRef)->fFreeSpace = (*pathRef)->currSize();
            (*pathRef)->fGenerationID = 0;
            (*pathRef)->validate();
        } else {
            int oldVCnt = (*pathRef)->countVerbs();
            int oldPCnt = (*pathRef)->countPoints();
            pathRef->reset(SkNEW(SkPathRef));
            (*pathRef)->resetToSize(0, 0, oldVCnt, oldPCnt);
        }
    }

    virtual ~SkPathRef() {
        this->validate();
        sk_free(fPoints);
    }

    int countPoints() const { this->validate(); return fPointCnt; }
    int countVerbs() const { this->validate(); return fVerbCnt; }

    


    const uint8_t* verbs() const { this->validate(); return fVerbs; }

    


    const uint8_t* verbsMemBegin() const { return this->verbs() - fVerbCnt; }

    


    const SkPoint* points() const { this->validate(); return fPoints; }

    


    const SkPoint* pointsEnd() const { return this->points() + this->countPoints(); }

    


    uint8_t atVerb(int index) {
        SkASSERT((unsigned) index < (unsigned) fVerbCnt);
        return this->verbs()[~index];
    }
    const SkPoint& atPoint(int index) const {
        SkASSERT((unsigned) index < (unsigned) fPointCnt);
        return this->points()[index];
    }

    bool operator== (const SkPathRef& ref) const {
        this->validate();
        ref.validate();
        bool genIDMatch = fGenerationID && fGenerationID == ref.fGenerationID;
#ifdef SK_RELEASE
        if (genIDMatch) {
            return true;
        }
#endif
        if (fPointCnt != ref.fPointCnt ||
            fVerbCnt != ref.fVerbCnt) {
            SkASSERT(!genIDMatch);
            return false;
        }
        if (0 != memcmp(this->verbsMemBegin(),
                        ref.verbsMemBegin(),
                        ref.fVerbCnt * sizeof(uint8_t))) {
            SkASSERT(!genIDMatch);
            return false;
        }
        if (0 != memcmp(this->points(),
                        ref.points(),
                        ref.fPointCnt * sizeof(SkPoint))) {
            SkASSERT(!genIDMatch);
            return false;
        }
        
        
        if (0 == fGenerationID) {
            fGenerationID = ref.genID();
        } else if (0 == ref.fGenerationID) {
            ref.fGenerationID = this->genID();
        }
        return true;
    }

    


#if NEW_PICTURE_FORMAT
    void writeToBuffer(SkWBuffer* buffer) {
        this->validate();
        SkDEBUGCODE(size_t beforePos = buffer->pos();)

        
        
        buffer->write32(0);
        buffer->write32(this->fVerbCnt);
        buffer->write32(this->fPointCnt);
        buffer->write(this->verbsMemBegin(), fVerbCnt * sizeof(uint8_t));
        buffer->write(fPoints, fPointCnt * sizeof(SkPoint));

        SkASSERT(buffer->pos() - beforePos == (size_t) this->writeSize());
    }

    


    uint32_t writeSize() {
        return 3 * sizeof(uint32_t) + fVerbCnt * sizeof(uint8_t) + fPointCnt * sizeof(SkPoint);
    }
#else
    void writeToBuffer(SkWBuffer* buffer) {
        this->validate();
        buffer->write(fPoints, fPointCnt * sizeof(SkPoint));
        for (int i = 0; i < fVerbCnt; ++i) {
            buffer->write8(fVerbs[~i]);
        }
    }
#endif

private:
    SkPathRef() {
        fPointCnt = 0;
        fVerbCnt = 0;
        fVerbs = NULL;
        fPoints = NULL;
        fFreeSpace = 0;
        fGenerationID = kEmptyGenID;
        SkDEBUGCODE(fEditorsAttached = 0;)
        this->validate();
    }

    void copy(const SkPathRef& ref, int additionalReserveVerbs, int additionalReservePoints) {
        this->validate();
        this->resetToSize(ref.fVerbCnt, ref.fPointCnt,
                          additionalReserveVerbs, additionalReservePoints);
        memcpy(this->verbsMemWritable(), ref.verbsMemBegin(), ref.fVerbCnt * sizeof(uint8_t));
        memcpy(this->fPoints, ref.fPoints, ref.fPointCnt * sizeof(SkPoint));
        
        
        fGenerationID = ref.fGenerationID;
        this->validate();
    }

    
    void incReserve(int additionalVerbs, int additionalPoints) {
        this->validate();
        size_t space = additionalVerbs * sizeof(uint8_t) + additionalPoints * sizeof (SkPoint);
        this->makeSpace(space);
        this->validate();
    }

    

    void resetToSize(int verbCount, int pointCount, int reserveVerbs = 0, int reservePoints = 0) {
        this->validate();
        fGenerationID = 0;

        size_t newSize = sizeof(uint8_t) * verbCount + sizeof(SkPoint) * pointCount;
        size_t newReserve = sizeof(uint8_t) * reserveVerbs + sizeof(SkPoint) * reservePoints;
        size_t minSize = newSize + newReserve;

        ptrdiff_t sizeDelta = this->currSize() - minSize;

        if (sizeDelta < 0 || static_cast<size_t>(sizeDelta) >= 3 * minSize) {
            sk_free(fPoints);
            fPoints = NULL;
            fVerbs = NULL;
            fFreeSpace = 0;
            fVerbCnt = 0;
            fPointCnt = 0;
            this->makeSpace(minSize);
            fVerbCnt = verbCount;
            fPointCnt = pointCount;
            fFreeSpace -= newSize;
        } else {
            fPointCnt = pointCount;
            fVerbCnt = verbCount;
            fFreeSpace = this->currSize() - minSize;
        }
        this->validate();
    }

    



    void grow(int newVerbs, int newPoints) {
        this->validate();
        size_t space = newVerbs * sizeof(uint8_t) + newPoints * sizeof (SkPoint);
        this->makeSpace(space);
        fVerbCnt += newVerbs;
        fPointCnt += newPoints;
        fFreeSpace -= space;
        this->validate();
    }

    




    SkPoint* growForVerb(SkPath::Verb verb) {
        this->validate();
        int pCnt;
        switch (verb) {
            case SkPath::kMove_Verb:
                pCnt = 1;
                break;
            case SkPath::kLine_Verb:
                pCnt = 1;
                break;
            case SkPath::kQuad_Verb:
                pCnt = 2;
                break;
            case SkPath::kCubic_Verb:
                pCnt = 3;
                break;
            default:
                pCnt = 0;
        }
        size_t space = sizeof(uint8_t) + pCnt * sizeof (SkPoint);
        this->makeSpace(space);
        this->fVerbs[~fVerbCnt] = verb;
        SkPoint* ret = fPoints + fPointCnt;
        fVerbCnt += 1;
        fPointCnt += pCnt;
        fFreeSpace -= space;
        this->validate();
        return ret;
    }

    



    void makeSpace(size_t size) {
        this->validate();
        ptrdiff_t growSize = size - fFreeSpace;
        if (growSize <= 0) {
            return;
        }
        size_t oldSize = this->currSize();
        
        growSize = (growSize + 7) & ~static_cast<size_t>(7);
        
        if (static_cast<size_t>(growSize) < oldSize) {
            growSize = oldSize;
        }
        if (growSize < kMinSize) {
            growSize = kMinSize;
        }
        size_t newSize = oldSize + growSize;
        
        
        fPoints = reinterpret_cast<SkPoint*>(sk_realloc_throw(fPoints, newSize));
        size_t oldVerbSize = fVerbCnt * sizeof(uint8_t);
        void* newVerbsDst = reinterpret_cast<void*>(
                                reinterpret_cast<intptr_t>(fPoints) + newSize - oldVerbSize);
        void* oldVerbsSrc = reinterpret_cast<void*>(
                                reinterpret_cast<intptr_t>(fPoints) + oldSize - oldVerbSize);
        memmove(newVerbsDst, oldVerbsSrc, oldVerbSize);
        fVerbs = reinterpret_cast<uint8_t*>(reinterpret_cast<intptr_t>(fPoints) + newSize);
        fFreeSpace += growSize;
        this->validate();
    }

    


    uint8_t* verbsMemWritable() {
        this->validate();
        return fVerbs - fVerbCnt;
    }

    


    size_t currSize() const {
        return reinterpret_cast<intptr_t>(fVerbs) - reinterpret_cast<intptr_t>(fPoints);
    }

    





    int32_t genID() const {
        SkDEBUGCODE(SkASSERT(!fEditorsAttached));
        if (!fGenerationID) {
            if (0 == fPointCnt && 0 == fVerbCnt) {
                fGenerationID = kEmptyGenID;
            } else {
                static int32_t  gPathRefGenerationID;
                
                
                do {
                    fGenerationID = sk_atomic_inc(&gPathRefGenerationID) + 1;
                } while (fGenerationID <= kEmptyGenID);
            }
        }
        return fGenerationID;
    }

    void validate() const {
        SkASSERT(static_cast<ptrdiff_t>(fFreeSpace) >= 0);
        SkASSERT(reinterpret_cast<intptr_t>(fVerbs) - reinterpret_cast<intptr_t>(fPoints) >= 0);
        SkASSERT((NULL == fPoints) == (NULL == fVerbs));
        SkASSERT(!(NULL == fPoints && 0 != fFreeSpace));
        SkASSERT(!(NULL == fPoints && 0 != fFreeSpace));
        SkASSERT(!(NULL == fPoints && fPointCnt));
        SkASSERT(!(NULL == fVerbs && fVerbCnt));
        SkASSERT(this->currSize() ==
                 fFreeSpace + sizeof(SkPoint) * fPointCnt + sizeof(uint8_t) * fVerbCnt);
    }

    enum {
        kMinSize = 256,
    };

    SkPoint*            fPoints; 
    uint8_t*            fVerbs; 
    int                 fVerbCnt;
    int                 fPointCnt;
    size_t              fFreeSpace; 
    enum {
        kEmptyGenID = 1, 
    };
    mutable int32_t     fGenerationID;
    SkDEBUGCODE(int32_t fEditorsAttached;) 

    typedef SkRefCnt INHERITED;
};

SK_DEFINE_INST_COUNT(SkPathRef);

#endif
