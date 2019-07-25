






































#ifndef jscell_h___
#define jscell_h___

#include "jspubtd.h"

struct JSCompartment;

namespace js {
namespace gc {

struct ArenaHeader;
struct Chunk;





static const uint32 BLACK = 0;




struct Cell {
    static const size_t CellShift = 3;
    static const size_t CellSize = size_t(1) << CellShift;
    static const size_t CellMask = CellSize - 1;

    inline uintptr_t address() const;
    inline ArenaHeader *arenaHeader() const;
    inline Chunk *chunk() const;

    JS_ALWAYS_INLINE bool isMarked(uint32 color = BLACK) const;
    JS_ALWAYS_INLINE bool markIfUnmarked(uint32 color = BLACK) const;
    JS_ALWAYS_INLINE void unmark(uint32 color) const;

    inline JSCompartment *compartment() const;

#ifdef DEBUG
    inline bool isAligned() const;
#endif
};

} 
} 

#endif 
