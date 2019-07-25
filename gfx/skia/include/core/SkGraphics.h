








#ifndef SkGraphics_DEFINED
#define SkGraphics_DEFINED

#include "SkTypes.h"

class SkGraphics {
public:
    static void Init();
    static void Term();

    

    static size_t GetFontCacheUsed();
    
    



    static bool SetFontCacheUsed(size_t usageInBytes);

    


    static void GetVersion(int32_t* major, int32_t* minor, int32_t* patch);

private:
    



    static void InstallNewHandler();
};

class SkAutoGraphics {
public:
    SkAutoGraphics() {
        SkGraphics::Init();
    }
    ~SkAutoGraphics() {
        SkGraphics::Term();
    }
};

#endif

