





#ifndef WEBRTC_MODULES_VIDEO_CAPTURE_MAIN_SOURCE_WINDOWS_DSHOWTOOLS_H_
#define WEBRTC_MODULES_VIDEO_CAPTURE_MAIN_SOURCE_WINDOWS_DSHOWTOOLS_H_

#include <assert.h>

namespace mozilla {







class CriticalSection
{
public:
    



    CriticalSection(const char* aName)
    {
      ::InitializeCriticalSection(&mCriticalSection);
    }

    


    ~CriticalSection()
    {
      ::DeleteCriticalSection(&mCriticalSection);
    }

    



    void Enter()
    {
      ::EnterCriticalSection(&mCriticalSection);
    }

    



    void Leave()
    {
      ::LeaveCriticalSection(&mCriticalSection);
    }

private:
    CriticalSection();
    CriticalSection(const CriticalSection&);
    CriticalSection& operator =(const CriticalSection&);

    CRITICAL_SECTION mCriticalSection;
};








 
class CriticalSectionAutoEnter
{
public:
    






    CriticalSectionAutoEnter(mozilla::CriticalSection &aCriticalSection) :
        mCriticalSection(&aCriticalSection)
    {
        assert(mCriticalSection);
        mCriticalSection->Enter();
    }
    
    ~CriticalSectionAutoEnter(void)
    {
        mCriticalSection->Leave();
    }
 

private:
    CriticalSectionAutoEnter();
    CriticalSectionAutoEnter(const CriticalSectionAutoEnter&);
    CriticalSectionAutoEnter& operator =(const CriticalSectionAutoEnter&);
    static void* operator new(size_t) throw();
    static void operator delete(void*);

    mozilla::CriticalSection* mCriticalSection;
};


} 


#endif 
