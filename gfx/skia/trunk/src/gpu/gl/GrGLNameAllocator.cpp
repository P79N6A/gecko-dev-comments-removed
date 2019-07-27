







#include "GrGLNameAllocator.h"








class GrGLNameAllocator::SparseNameRange : public SkRefCnt {
public:
    virtual ~SparseNameRange() {}

    




    GrGLuint first() const { return fFirst; }

    




    GrGLuint end() const { return fEnd; }

    





    GrGLuint height() const { return fHeight; }

    











    virtual SparseNameRange* SK_WARN_UNUSED_RESULT internalAllocate(GrGLuint* outName) = 0;

    














    virtual SparseNameRange* SK_WARN_UNUSED_RESULT removeLeftmostContiguousRange(GrGLuint* removedCount) = 0;

    







    virtual GrGLuint appendNames(GrGLuint count) = 0;

    








    virtual GrGLuint prependNames(GrGLuint count) = 0;

    












    virtual SparseNameRange* SK_WARN_UNUSED_RESULT free(GrGLuint name) = 0;

protected:
    SparseNameRange* takeRef() {
      this->ref();
      return this;
    }

    GrGLuint fFirst;
    GrGLuint fEnd;
    GrGLuint fHeight;
};





class GrGLNameAllocator::SparseNameTree : public SparseNameRange {
public:
    SparseNameTree(SparseNameRange* left, SparseNameRange* right)
        : fLeft(left),
          fRight(right) {
        SkASSERT(fLeft.get());
        SkASSERT(fRight.get());
        this->updateStats();
    }

    virtual SparseNameRange* SK_WARN_UNUSED_RESULT internalAllocate(GrGLuint* outName) SK_OVERRIDE {
        
        fLeft.reset(fLeft->internalAllocate(outName));
        if (0 != *outName) {
            this->updateStats();
            return this->rebalance();
        }

        if (fLeft->end() + 1 == fRight->first()) {
            
            GrGLuint removedCount;
            fRight.reset(fRight->removeLeftmostContiguousRange(&removedCount));
            *outName = fLeft->appendNames(1 + removedCount);
            if (NULL == fRight.get()) {
                return fLeft.detach();
            }
            this->updateStats();
            return this->rebalance();
        }

        
        
        SkASSERT(fLeft->end() + 1 < fRight->first());
        *outName = fLeft->appendNames(1);
        return this->takeRef();
    }

    virtual SparseNameRange* SK_WARN_UNUSED_RESULT removeLeftmostContiguousRange(GrGLuint* removedCount) SK_OVERRIDE {
        fLeft.reset(fLeft->removeLeftmostContiguousRange(removedCount));
        if (NULL == fLeft) {
            return fRight.detach();
        }
        this->updateStats();
        return this->rebalance();
    }

    virtual GrGLuint appendNames(GrGLuint count) SK_OVERRIDE {
        SkASSERT(fEnd + count > fEnd); 
        GrGLuint name = fRight->appendNames(count);
        SkASSERT(fRight->end() == fEnd + count);
        this->updateStats();
        return name;
    }

    virtual GrGLuint prependNames(GrGLuint count) SK_OVERRIDE {
        SkASSERT(fFirst > count); 
        GrGLuint name = fLeft->prependNames(count);
        SkASSERT(fLeft->first() == fFirst - count);
        this->updateStats();
        return name;
    }

    virtual SparseNameRange* SK_WARN_UNUSED_RESULT free(GrGLuint name) SK_OVERRIDE {
        if (name < fLeft->end()) {
            fLeft.reset(fLeft->free(name));
            if (NULL == fLeft) {
                
                return fRight.detach();
            }
            this->updateStats();
            return this->rebalance();
        } else {
            fRight.reset(fRight->free(name));
            if (NULL == fRight) {
                
                return fLeft.detach();
            }
            this->updateStats();
            return this->rebalance();
        }
    }

private:
    typedef SkAutoTUnref<SparseNameRange> SparseNameTree::* ChildRange;

    SparseNameRange* SK_WARN_UNUSED_RESULT rebalance() {
        if (fLeft->height() > fRight->height() + 1) {
            return this->rebalanceImpl<&SparseNameTree::fLeft, &SparseNameTree::fRight>();
        }
        if (fRight->height() > fLeft->height() + 1) {
            return this->rebalanceImpl<&SparseNameTree::fRight, &SparseNameTree::fLeft>();
        }
        return this->takeRef();
    }

    



    template<ChildRange Tall, ChildRange Short>
    SparseNameRange* SK_WARN_UNUSED_RESULT rebalanceImpl() {
        
        
        SkASSERT(2 == (this->*Tall)->height() - (this->*Short)->height());

        
        
        SparseNameTree* tallChild = static_cast<SparseNameTree*>((this->*Tall).get());
        if ((tallChild->*Tall)->height() < (tallChild->*Short)->height()) {
            (this->*Tall).reset(tallChild->rotate<Short, Tall>());
        }

        
        return this->rotate<Tall, Short>();
    }

    



    template<ChildRange Tall, ChildRange Short>
    SparseNameRange* SK_WARN_UNUSED_RESULT rotate() {
        SparseNameTree* newRoot = static_cast<SparseNameTree*>((this->*Tall).detach());

        (this->*Tall).reset((newRoot->*Short).detach());
        this->updateStats();

        (newRoot->*Short).reset(this->takeRef());
        newRoot->updateStats();

        return newRoot;
    }

    void updateStats() {
        SkASSERT(fLeft->end() < fRight->first()); 
        fFirst = fLeft->first();
        fEnd = fRight->end();
        fHeight = 1 + SkMax32(fLeft->height(), fRight->height());
    }

    SkAutoTUnref<SparseNameRange> fLeft;
    SkAutoTUnref<SparseNameRange> fRight;
};





class GrGLNameAllocator::ContiguousNameRange : public SparseNameRange {
public:
    ContiguousNameRange(GrGLuint first, GrGLuint end) {
        SkASSERT(first < end);
        fFirst = first;
        fEnd = end;
        fHeight = 0;
    }

    virtual SparseNameRange* SK_WARN_UNUSED_RESULT internalAllocate(GrGLuint* outName) SK_OVERRIDE {
        *outName = 0; 
        return this->takeRef();
    }

    virtual SparseNameRange* SK_WARN_UNUSED_RESULT removeLeftmostContiguousRange(GrGLuint* removedCount) SK_OVERRIDE {
        *removedCount = fEnd - fFirst;
        return NULL;
    }

    virtual GrGLuint appendNames(GrGLuint count) SK_OVERRIDE {
        SkASSERT(fEnd + count > fEnd); 
        GrGLuint name = fEnd;
        fEnd += count;
        return name;
    }

    virtual GrGLuint prependNames(GrGLuint count) SK_OVERRIDE {
        SkASSERT(fFirst > count); 
        fFirst -= count;
        return fFirst;
    }

    virtual SparseNameRange* SK_WARN_UNUSED_RESULT free(GrGLuint name) SK_OVERRIDE {
        if (name < fFirst || name >= fEnd) {
          
          return this->takeRef();
        }

        if (fFirst == name) {
            ++fFirst;
            return (fEnd == fFirst) ? NULL : this->takeRef();
        }

        if (fEnd == name + 1) {
            --fEnd;
            return this->takeRef();
        }

        SparseNameRange* left = SkNEW_ARGS(ContiguousNameRange, (fFirst, name));
        SparseNameRange* right = this->takeRef();
        fFirst = name + 1;
        return SkNEW_ARGS(SparseNameTree, (left, right));
    }
};

GrGLNameAllocator::GrGLNameAllocator(GrGLuint firstName, GrGLuint endName)
    : fFirstName(firstName),
      fEndName(endName) {
    SkASSERT(firstName > 0);
    SkASSERT(endName > firstName);
}

GrGLNameAllocator::~GrGLNameAllocator() {
}

GrGLuint GrGLNameAllocator::allocateName() {
    if (NULL == fAllocatedNames.get()) {
        fAllocatedNames.reset(SkNEW_ARGS(ContiguousNameRange, (fFirstName, fFirstName + 1)));
        return fFirstName;
    }

    if (fAllocatedNames->first() > fFirstName) {
        return fAllocatedNames->prependNames(1);
    }

    GrGLuint name;
    fAllocatedNames.reset(fAllocatedNames->internalAllocate(&name));
    if (0 != name) {
        return name;
    }

    if (fAllocatedNames->end() < fEndName) {
        return fAllocatedNames->appendNames(1);
    }

    
    return 0;
}

void GrGLNameAllocator::free(GrGLuint name) {
    if (!fAllocatedNames.get()) {
      
      return;
    }

    fAllocatedNames.reset(fAllocatedNames->free(name));
}
