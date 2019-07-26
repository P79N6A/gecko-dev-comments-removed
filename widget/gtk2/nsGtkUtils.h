






#ifndef nsGtkUtils_h__
#define nsGtkUtils_h__

#include <glib.h>



template<class T> static inline gpointer
FuncToGpointer(T aFunction)
{
    return reinterpret_cast<gpointer>
        (reinterpret_cast<uintptr_t>
         
         (reinterpret_cast<void (*)()>(aFunction)));
}

#endif 
