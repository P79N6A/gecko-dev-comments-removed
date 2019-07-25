






































#ifndef jscell_h___
#define jscell_h___

struct JSCompartment;

namespace js {
namespace gc {

template <typename T> struct Arena;
struct ArenaBitmap;
struct MarkingDelay;
struct Chunk;
struct FreeCell;









struct Cell {
    static const size_t CellShift = 3;
    static const size_t CellSize = size_t(1) << CellShift;
    static const size_t CellMask = CellSize - 1;

    inline Arena<Cell> *arena() const;
    inline Chunk *chunk() const;
    inline ArenaBitmap *bitmap() const;
    JS_ALWAYS_INLINE size_t cellIndex() const;

    JS_ALWAYS_INLINE bool isMarked(uint32 color) const;
    JS_ALWAYS_INLINE bool markIfUnmarked(uint32 color) const;

    inline JSCompartment *compartment() const;

    
    JS_ALWAYS_INLINE js::gc::Cell *asCell() { return this; }

    JS_ALWAYS_INLINE js::gc::FreeCell *asFreeCell() {
        return reinterpret_cast<FreeCell *>(this);
    }
};


struct FreeCell : Cell {
    union {
        FreeCell *link;
        double data;
    };
};

JS_STATIC_ASSERT(sizeof(FreeCell) == 8);

} 
} 

#endif 
