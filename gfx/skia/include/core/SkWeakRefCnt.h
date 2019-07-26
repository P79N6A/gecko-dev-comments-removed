






#ifndef SkWeakRefCnt_DEFINED
#define SkWeakRefCnt_DEFINED

#include "SkRefCnt.h"
#include "SkThread.h"







































class SK_API SkWeakRefCnt : public SkRefCnt {
public:
    SK_DECLARE_INST_COUNT(SkWeakRefCnt)

    




    SkWeakRefCnt() : SkRefCnt(), fWeakCnt(1) {}

    

    virtual ~SkWeakRefCnt() {
#ifdef SK_DEBUG
        SkASSERT(fWeakCnt == 1);
        fWeakCnt = 0;
#endif
    }

    

    int32_t getWeakCnt() const { return fWeakCnt; }

    void validate() const {
        SkRefCnt::validate();
        SkASSERT(fWeakCnt > 0);
    }

    






    bool SK_WARN_UNUSED_RESULT try_ref() const {
        if (sk_atomic_conditional_inc(&fRefCnt) != 0) {
            
            
            sk_membar_aquire__after_atomic_conditional_inc();
            return true;
        }
        return false;
    }

    


    void weak_ref() const {
        SkASSERT(fRefCnt > 0);
        SkASSERT(fWeakCnt > 0);
        sk_atomic_inc(&fWeakCnt);  
    }

    




    void weak_unref() const {
        SkASSERT(fWeakCnt > 0);
        
        if (sk_atomic_dec(&fWeakCnt) == 1) {
            
            
            sk_membar_aquire__after_atomic_dec();
#ifdef SK_DEBUG
            
            fWeakCnt = 1;
#endif
            SkRefCnt::internal_dispose();
        }
    }

    


    bool weak_expired() const {
        return fRefCnt == 0;
    }

protected:
    




    virtual void weak_dispose() const {
    }

private:
    



    virtual void internal_dispose() const SK_OVERRIDE {
        weak_dispose();
        weak_unref();
    }

    
    mutable int32_t fWeakCnt;

    typedef SkRefCnt INHERITED;
};

#endif
