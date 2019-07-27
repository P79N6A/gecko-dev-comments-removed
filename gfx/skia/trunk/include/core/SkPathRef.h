







#ifndef SkPathRef_DEFINED
#define SkPathRef_DEFINED

#include "SkDynamicAnnotations.h"
#include "SkMatrix.h"
#include "SkPoint.h"
#include "SkRect.h"
#include "SkRefCnt.h"
#include "SkTDArray.h"
#include <stddef.h> 

class SkRBuffer;
class SkWBuffer;
















class SK_API SkPathRef : public ::SkRefCnt {
public:
    SK_DECLARE_INST_COUNT(SkPathRef);

    class Editor {
    public:
        Editor(SkAutoTUnref<SkPathRef>* pathRef,
               int incReserveVerbs = 0,
               int incReservePoints = 0);

        ~Editor() { SkDEBUGCODE(sk_atomic_dec(&fPathRef->fEditorsAttached);) }

        


        SkPoint* points() { return fPathRef->getPoints(); }
        const SkPoint* points() const { return fPathRef->points(); }

        


        SkPoint* atPoint(int i) {
            SkASSERT((unsigned) i < (unsigned) fPathRef->fPointCnt);
            return this->points() + i;
        };
        const SkPoint* atPoint(int i) const {
            SkASSERT((unsigned) i < (unsigned) fPathRef->fPointCnt);
            return this->points() + i;
        };

        




        SkPoint* growForVerb(int  verb, SkScalar weight = 0) {
            SkDEBUGCODE(fPathRef->validate();)
            return fPathRef->growForVerb(verb, weight);
        }

        






        SkPoint* growForRepeatedVerb(int  verb,
                                     int numVbs,
                                     SkScalar** weights = NULL) {
            return fPathRef->growForRepeatedVerb(verb, numVbs, weights);
        }

        



        void resetToSize(int newVerbCnt, int newPointCnt, int newConicCount) {
            fPathRef->resetToSize(newVerbCnt, newPointCnt, newConicCount);
        }

        


        SkPathRef* pathRef() { return fPathRef; }

        void setIsOval(bool isOval) { fPathRef->setIsOval(isOval); }

        void setBounds(const SkRect& rect) { fPathRef->setBounds(rect); }

    private:
        SkPathRef* fPathRef;
    };

public:
    


    static SkPathRef* CreateEmpty();

    



    bool isFinite() const {
        if (fBoundsIsDirty) {
            this->computeBounds();
        }
        return SkToBool(fIsFinite);
    }

    




    uint32_t getSegmentMasks() const { return fSegmentMask; }

    









    bool isOval(SkRect* rect) const {
        if (fIsOval && NULL != rect) {
            *rect = getBounds();
        }

        return SkToBool(fIsOval);
    }

    bool hasComputedBounds() const {
        return !fBoundsIsDirty;
    }

    




    const SkRect& getBounds() const {
        if (fBoundsIsDirty) {
            this->computeBounds();
        }
        return fBounds;
    }

    


    static void CreateTransformedCopy(SkAutoTUnref<SkPathRef>* dst,
                                      const SkPathRef& src,
                                      const SkMatrix& matrix);

    static SkPathRef* CreateFromBuffer(SkRBuffer* buffer);

    




    static void Rewind(SkAutoTUnref<SkPathRef>* pathRef);

    virtual ~SkPathRef() {
        SkDEBUGCODE(this->validate();)
        sk_free(fPoints);

        SkDEBUGCODE(fPoints = NULL;)
        SkDEBUGCODE(fVerbs = NULL;)
        SkDEBUGCODE(fVerbCnt = 0x9999999;)
        SkDEBUGCODE(fPointCnt = 0xAAAAAAA;)
        SkDEBUGCODE(fPointCnt = 0xBBBBBBB;)
        SkDEBUGCODE(fGenerationID = 0xEEEEEEEE;)
        SkDEBUGCODE(fEditorsAttached = 0x7777777;)
    }

    int countPoints() const { SkDEBUGCODE(this->validate();) return fPointCnt; }
    int countVerbs() const { SkDEBUGCODE(this->validate();) return fVerbCnt; }
    int countWeights() const { SkDEBUGCODE(this->validate();) return fConicWeights.count(); }

    


    const uint8_t* verbs() const { SkDEBUGCODE(this->validate();) return fVerbs; }

    


    const uint8_t* verbsMemBegin() const { return this->verbs() - fVerbCnt; }

    


    const SkPoint* points() const { SkDEBUGCODE(this->validate();) return fPoints; }

    


    const SkPoint* pointsEnd() const { return this->points() + this->countPoints(); }

    const SkScalar* conicWeights() const { SkDEBUGCODE(this->validate();) return fConicWeights.begin(); }
    const SkScalar* conicWeightsEnd() const { SkDEBUGCODE(this->validate();) return fConicWeights.end(); }

    


    uint8_t atVerb(int index) const {
        SkASSERT((unsigned) index < (unsigned) fVerbCnt);
        return this->verbs()[~index];
    }
    const SkPoint& atPoint(int index) const {
        SkASSERT((unsigned) index < (unsigned) fPointCnt);
        return this->points()[index];
    }

    bool operator== (const SkPathRef& ref) const;

    


    void writeToBuffer(SkWBuffer* buffer) const;

    


    uint32_t writeSize() const;

    




    uint32_t genID() const;

private:
    enum SerializationOffsets {
        kIsFinite_SerializationShift = 25,  
        kIsOval_SerializationShift = 24,    
        kSegmentMask_SerializationShift = 0 
    };

    SkPathRef() {
        fBoundsIsDirty = true;    
        fPointCnt = 0;
        fVerbCnt = 0;
        fVerbs = NULL;
        fPoints = NULL;
        fFreeSpace = 0;
        fGenerationID = kEmptyGenID;
        fSegmentMask = 0;
        fIsOval = false;
        SkDEBUGCODE(fEditorsAttached = 0;)
        SkDEBUGCODE(this->validate();)
    }

    void copy(const SkPathRef& ref, int additionalReserveVerbs, int additionalReservePoints);

    
    static bool ComputePtBounds(SkRect* bounds, const SkPathRef& ref) {
        int count = ref.countPoints();
        if (count <= 1) {  
            bounds->setEmpty();
            return count ? ref.points()->isFinite() : true;
        } else {
            return bounds->setBoundsCheck(ref.points(), count);
        }
    }

    
    void computeBounds() const {
        SkDEBUGCODE(this->validate();)
        
        
        

        fIsFinite = ComputePtBounds(fBounds.get(), *this);
        fBoundsIsDirty = false;
    }

    void setBounds(const SkRect& rect) {
        SkASSERT(rect.fLeft <= rect.fRight && rect.fTop <= rect.fBottom);
        fBounds = rect;
        fBoundsIsDirty = false;
        fIsFinite = fBounds->isFinite();
    }

    
    void incReserve(int additionalVerbs, int additionalPoints) {
        SkDEBUGCODE(this->validate();)
        size_t space = additionalVerbs * sizeof(uint8_t) + additionalPoints * sizeof (SkPoint);
        this->makeSpace(space);
        SkDEBUGCODE(this->validate();)
    }

    

    void resetToSize(int verbCount, int pointCount, int conicCount,
                     int reserveVerbs = 0, int reservePoints = 0) {
        SkDEBUGCODE(this->validate();)
        fBoundsIsDirty = true;      
        fGenerationID = 0;

        fSegmentMask = 0;
        fIsOval = false;

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
        fConicWeights.setCount(conicCount);
        SkDEBUGCODE(this->validate();)
    }

    





    SkPoint* growForRepeatedVerb(int  verb, int numVbs, SkScalar** weights);

    




    SkPoint* growForVerb(int  verb, SkScalar weight);

    



    void makeSpace(size_t size) {
        SkDEBUGCODE(this->validate();)
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
        SkDEBUGCODE(this->validate();)
    }

    


    uint8_t* verbsMemWritable() {
        SkDEBUGCODE(this->validate();)
        return fVerbs - fVerbCnt;
    }

    


    size_t currSize() const {
        return reinterpret_cast<intptr_t>(fVerbs) - reinterpret_cast<intptr_t>(fPoints);
    }

    SkDEBUGCODE(void validate() const;)

    


    static SkPathRef* CreateEmptyImpl();

    void setIsOval(bool isOval) { fIsOval = isOval; }

    SkPoint* getPoints() {
        SkDEBUGCODE(this->validate();)
        fIsOval = false;
        return fPoints;
    }

    enum {
        kMinSize = 256,
    };

    mutable SkTRacyReffable<SkRect> fBounds;
    mutable SkTRacy<uint8_t>        fBoundsIsDirty;
    mutable SkTRacy<SkBool8>        fIsFinite;    

    SkBool8  fIsOval;
    uint8_t  fSegmentMask;

    SkPoint*            fPoints; 
    uint8_t*            fVerbs; 
    int                 fVerbCnt;
    int                 fPointCnt;
    size_t              fFreeSpace; 
    SkTDArray<SkScalar> fConicWeights;

    enum {
        kEmptyGenID = 1, 
    };
    mutable uint32_t    fGenerationID;
    SkDEBUGCODE(int32_t fEditorsAttached;) 

    friend class PathRefTest_Private;
    typedef SkRefCnt INHERITED;
};

#endif
