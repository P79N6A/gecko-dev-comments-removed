







































#ifndef mozilla_BlockingResourceBase_h
#define mozilla_BlockingResourceBase_h

#include "prlock.h"
#include "prlog.h"

#include "nscore.h"
#include "nsDebug.h"
#include "nsError.h"
#include "nsTraceRefcnt.h"

#ifdef DEBUG
#include "prinit.h"
#include "prthread.h"

#include "nsStringGlue.h"

#include "mozilla/DeadlockDetector.h"
#include "nsXPCOM.h"
#endif





namespace mozilla {







class NS_COM_GLUE BlockingResourceBase
{
public:
    
    enum BlockingResourceType { eMutex, eReentrantMonitor, eCondVar };

    



    static const char* const kResourceTypeName[];


#ifdef DEBUG

private:
    
    struct DeadlockDetectorEntry;

    
    typedef DeadlockDetector<DeadlockDetectorEntry> DDT;

    








    struct DeadlockDetectorEntry
    {
        DeadlockDetectorEntry(const char* aName,
                              BlockingResourceType aType) :
            mName(aName),
            mType(aType),
            mAcquisitionContext(CallStack::kNone)
        {
            NS_ABORT_IF_FALSE(mName, "Name must be nonnull");
        }
        
        


















        bool Print(const DDT::ResourceAcquisition& aFirstSeen,
                   nsACString& out,
                   bool aPrintFirstSeenCx=false) const;

        




        const char* mName;
        




        BlockingResourceType mType;
        




        CallStack mAcquisitionContext;
    };

protected:
    










    BlockingResourceBase(const char* aName, BlockingResourceType aType);

    ~BlockingResourceBase();

    







    void CheckAcquire(const CallStack& aCallContext);

    







    void Acquire(const CallStack& aCallContext); 

    








    void Release();             

    













    static bool PrintCycle(const DDT::ResourceAcquisitionArray* cycle,
                           nsACString& out);

    







    static BlockingResourceBase* ResourceChainFront()
    {
        return (BlockingResourceBase*)
            PR_GetThreadPrivate(sResourceAcqnChainFrontTPI);
    }

    




    static BlockingResourceBase*
    ResourceChainPrev(const BlockingResourceBase* aResource)
    {
        return aResource->mChainPrev;
    } 

    






    void ResourceChainAppend(BlockingResourceBase* aPrev)
    {
        mChainPrev = aPrev;
        PR_SetThreadPrivate(sResourceAcqnChainFrontTPI, this);
    } 

    





    void ResourceChainRemove()
    {
        NS_ASSERTION(this == ResourceChainFront(), "not at chain front");
        PR_SetThreadPrivate(sResourceAcqnChainFrontTPI, mChainPrev);
    } 

    






    CallStack
    GetAcquisitionContext()
    {
        return mDDEntry->mAcquisitionContext;
    }

    





    void
    SetAcquisitionContext(CallStack aAcquisitionContext)
    {
        mDDEntry->mAcquisitionContext = aAcquisitionContext;
    }

    





    BlockingResourceBase* mChainPrev;

private:
    



    DeadlockDetectorEntry* mDDEntry;

    




    static PRCallOnceType sCallOnce;

    




    static PRUintn sResourceAcqnChainFrontTPI;

    



    static DDT* sDeadlockDetector;

    






    static PRStatus InitStatics() {
        PR_NewThreadPrivateIndex(&sResourceAcqnChainFrontTPI, 0);
        sDeadlockDetector = new DDT();
        if (!sDeadlockDetector)
            NS_RUNTIMEABORT("can't allocate deadlock detector");
        return PR_SUCCESS;
    }

    





    static void Shutdown() {
        delete sDeadlockDetector;
        sDeadlockDetector = 0;
    }

#  ifdef MOZILLA_INTERNAL_API
    
    friend void LogTerm();
#  endif  

#else  

    BlockingResourceBase(const char* aName, BlockingResourceType aType)
    {
    }

    ~BlockingResourceBase()
    {
    }

#endif
};


} 


#endif
