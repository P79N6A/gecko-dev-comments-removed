




























#ifndef assembler_assembler_MacroAssemblerX86Common_h
#define assembler_assembler_MacroAssemblerX86Common_h

#include "assembler/assembler/X86Assembler.h"

namespace JSC {

class MacroAssemblerX86Common {
public:
    
    
    enum SSECheckState {
        NotCheckedSSE = 0,
        NoSSE = 1,
        HasSSE = 2,
        HasSSE2 = 3,
        HasSSE3 = 4,
        HasSSSE3 = 5,
        HasSSE4_1 = 6,
        HasSSE4_2 = 7
    };

    static SSECheckState getSSEState()
    {
        if (s_sseCheckState == NotCheckedSSE) {
            MacroAssemblerX86Common::setSSECheckState();
        }
        
        MOZ_ASSERT(s_sseCheckState != NotCheckedSSE);

        return s_sseCheckState;
    }

private:
    static SSECheckState s_sseCheckState;

    static void setSSECheckState();

  public:
#ifdef JS_CODEGEN_X86
    static bool isSSEPresent()
    {
#if defined(__SSE__) && !defined(DEBUG)
        return true;
#else
        if (s_sseCheckState == NotCheckedSSE) {
            setSSECheckState();
        }
        
        MOZ_ASSERT(s_sseCheckState != NotCheckedSSE);

        return s_sseCheckState >= HasSSE;
#endif
    }

    static bool isSSE2Present()
    {
#if defined(__SSE2__) && !defined(DEBUG)
        return true;
#else
        if (s_sseCheckState == NotCheckedSSE) {
            setSSECheckState();
        }
        
        MOZ_ASSERT(s_sseCheckState != NotCheckedSSE);

        return s_sseCheckState >= HasSSE2;
#endif
    }

#elif !defined(NDEBUG) 

    
    
    static bool isSSE2Present()
    {
        return true;
    }

#endif
    static bool isSSE3Present()
    {
#if defined(__SSE3__) && !defined(DEBUG)
        return true;
#else
        if (s_sseCheckState == NotCheckedSSE) {
            setSSECheckState();
        }
        
        MOZ_ASSERT(s_sseCheckState != NotCheckedSSE);

        return s_sseCheckState >= HasSSE3;
#endif
    }

    static bool isSSSE3Present()
    {
#if defined(__SSSE3__) && !defined(DEBUG)
        return true;
#else
        if (s_sseCheckState == NotCheckedSSE) {
            setSSECheckState();
        }
        
        MOZ_ASSERT(s_sseCheckState != NotCheckedSSE);

        return s_sseCheckState >= HasSSSE3;
#endif
    }

    static bool isSSE41Present()
    {
#if defined(__SSE4_1__) && !defined(DEBUG)
        return true;
#else
        if (s_sseCheckState == NotCheckedSSE) {
            setSSECheckState();
        }
        
        MOZ_ASSERT(s_sseCheckState != NotCheckedSSE);

        return s_sseCheckState >= HasSSE4_1;
#endif
    }

    static bool isSSE42Present()
    {
#if defined(__SSE4_2__) && !defined(DEBUG)
        return true;
#else
        if (s_sseCheckState == NotCheckedSSE) {
            setSSECheckState();
        }
        
        MOZ_ASSERT(s_sseCheckState != NotCheckedSSE);

        return s_sseCheckState >= HasSSE4_2;
#endif
    }

  private:
#ifdef DEBUG
    static bool s_floatingPointDisabled;
    static bool s_SSE3Disabled;
    static bool s_SSE4Disabled;

  public:
    static void SetFloatingPointDisabled() {
        s_floatingPointDisabled = true;
    }
    static void SetSSE3Disabled() {
        s_SSE3Disabled = true;
    }
    static void SetSSE4Disabled() {
        s_SSE4Disabled = true;
    }
#endif
};

} 

#endif 
