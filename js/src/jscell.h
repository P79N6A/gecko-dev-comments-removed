






































#ifndef jscell_h___
#define jscell_h___

#include "jspubtd.h"

struct JSCompartment;

namespace js {
namespace gc {

struct ArenaHeader;
struct Chunk;


enum AllocKind {
    FINALIZE_OBJECT0,
    FINALIZE_OBJECT0_BACKGROUND,
    FINALIZE_OBJECT2,
    FINALIZE_OBJECT2_BACKGROUND,
    FINALIZE_OBJECT4,
    FINALIZE_OBJECT4_BACKGROUND,
    FINALIZE_OBJECT8,
    FINALIZE_OBJECT8_BACKGROUND,
    FINALIZE_OBJECT12,
    FINALIZE_OBJECT12_BACKGROUND,
    FINALIZE_OBJECT16,
    FINALIZE_OBJECT16_BACKGROUND,
    FINALIZE_OBJECT_LAST = FINALIZE_OBJECT16_BACKGROUND,
    FINALIZE_FUNCTION,
    FINALIZE_FUNCTION_AND_OBJECT_LAST = FINALIZE_FUNCTION,
    FINALIZE_SCRIPT,
    FINALIZE_SHAPE,
    FINALIZE_BASE_SHAPE,
    FINALIZE_TYPE_OBJECT,
#if JS_HAS_XML_SUPPORT
    FINALIZE_XML,
#endif
    FINALIZE_SHORT_STRING,
    FINALIZE_STRING,
    FINALIZE_EXTERNAL_STRING,
    FINALIZE_LAST = FINALIZE_EXTERNAL_STRING
};

const size_t FINALIZE_LIMIT = FINALIZE_LAST + 1;






static const uint32 BLACK = 0;
static const uint32 GRAY = 1;




struct Cell {
    static const size_t CellShift = 3;
    static const size_t CellSize = size_t(1) << CellShift;
    static const size_t CellMask = CellSize - 1;

    inline uintptr_t address() const;
    inline ArenaHeader *arenaHeader() const;
    inline Chunk *chunk() const;
    inline AllocKind getAllocKind() const;

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
