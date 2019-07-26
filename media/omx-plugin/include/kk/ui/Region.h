















#ifndef ANDROID_UI_REGION_H
#define ANDROID_UI_REGION_H

#include <stdint.h>
#include <sys/types.h>

#include <utils/Vector.h>

#include <ui/Rect.h>
#include <utils/Flattenable.h>

namespace android {


class SharedBuffer;
class String8;


class Region : public LightFlattenable<Region>
{
public:
                        Region();
                        Region(const Region& rhs);
    explicit            Region(const Rect& rhs);
                        ~Region();

    static  Region      createTJunctionFreeRegion(const Region& r);

        Region& operator = (const Region& rhs);

    inline  bool        isEmpty() const     { return getBounds().isEmpty(); }
    inline  bool        isRect() const      { return mStorage.size() == 1; }

    inline  Rect        getBounds() const   { return mStorage[mStorage.size() - 1]; }
    inline  Rect        bounds() const      { return getBounds(); }

            
            Region&     makeBoundsSelf();
    
            void        clear();
            void        set(const Rect& r);
            void        set(uint32_t w, uint32_t h);
        
            Region&     orSelf(const Rect& rhs);
            Region&     xorSelf(const Rect& rhs);
            Region&     andSelf(const Rect& rhs);
            Region&     subtractSelf(const Rect& rhs);

            
            Region&     orSelf(const Region& rhs);
            Region&     xorSelf(const Region& rhs);
            Region&     andSelf(const Region& rhs);
            Region&     subtractSelf(const Region& rhs);

            
    const   Region      merge(const Rect& rhs) const;
    const   Region      mergeExclusive(const Rect& rhs) const;
    const   Region      intersect(const Rect& rhs) const;
    const   Region      subtract(const Rect& rhs) const;

            
    const   Region      merge(const Region& rhs) const;
    const   Region      mergeExclusive(const Region& rhs) const;
    const   Region      intersect(const Region& rhs) const;
    const   Region      subtract(const Region& rhs) const;

            
            Region&     translateSelf(int dx, int dy);
            Region&     orSelf(const Region& rhs, int dx, int dy);
            Region&     xorSelf(const Region& rhs, int dx, int dy);
            Region&     andSelf(const Region& rhs, int dx, int dy);
            Region&     subtractSelf(const Region& rhs, int dx, int dy);

            
    const   Region      translate(int dx, int dy) const;
    const   Region      merge(const Region& rhs, int dx, int dy) const;
    const   Region      mergeExclusive(const Region& rhs, int dx, int dy) const;
    const   Region      intersect(const Region& rhs, int dx, int dy) const;
    const   Region      subtract(const Region& rhs, int dx, int dy) const;

    
    inline  const Region      operator | (const Region& rhs) const;
    inline  const Region      operator ^ (const Region& rhs) const;
    inline  const Region      operator & (const Region& rhs) const;
    inline  const Region      operator - (const Region& rhs) const;
    inline  const Region      operator + (const Point& pt) const;

    inline  Region&     operator |= (const Region& rhs);
    inline  Region&     operator ^= (const Region& rhs);
    inline  Region&     operator &= (const Region& rhs);
    inline  Region&     operator -= (const Region& rhs);
    inline  Region&     operator += (const Point& pt);

    
    
    bool isTriviallyEqual(const Region& region) const;


    

    
    
    typedef Rect const* const_iterator;
    const_iterator begin() const;
    const_iterator end() const;

    
    
    Rect const* getArray(size_t* count) const;

    
    
    
    SharedBuffer const* getSharedBuffer(size_t* count) const;

    
            
            
            
            void        addRectUnchecked(int l, int t, int r, int b);

    inline  bool        isFixedSize() const { return false; }
            size_t      getSize() const;
            status_t    flatten(void* buffer) const;
            status_t    unflatten(void const* buffer, size_t size);

    void        dump(String8& out, const char* what, uint32_t flags=0) const;
    void        dump(const char* what, uint32_t flags=0) const;

private:
    class rasterizer;
    friend class rasterizer;
    
    Region& operationSelf(const Rect& r, int op);
    Region& operationSelf(const Region& r, int op);
    Region& operationSelf(const Region& r, int dx, int dy, int op);
    const Region operation(const Rect& rhs, int op) const;
    const Region operation(const Region& rhs, int op) const;
    const Region operation(const Region& rhs, int dx, int dy, int op) const;

    static void boolean_operation(int op, Region& dst,
            const Region& lhs, const Region& rhs, int dx, int dy);
    static void boolean_operation(int op, Region& dst,
            const Region& lhs, const Rect& rhs, int dx, int dy);

    static void boolean_operation(int op, Region& dst,
            const Region& lhs, const Region& rhs);
    static void boolean_operation(int op, Region& dst,
            const Region& lhs, const Rect& rhs);

    static void translate(Region& reg, int dx, int dy);
    static void translate(Region& dst, const Region& reg, int dx, int dy);

    static bool validate(const Region& reg,
            const char* name, bool silent = false);
    
    
    
    
    
    Vector<Rect> mStorage;
};


const Region Region::operator | (const Region& rhs) const {
    return merge(rhs);
}
const Region Region::operator ^ (const Region& rhs) const {
    return mergeExclusive(rhs);
}
const Region Region::operator & (const Region& rhs) const {
    return intersect(rhs);
}
const Region Region::operator - (const Region& rhs) const {
    return subtract(rhs);
}
const Region Region::operator + (const Point& pt) const {
    return translate(pt.x, pt.y);
}


Region& Region::operator |= (const Region& rhs) {
    return orSelf(rhs);
}
Region& Region::operator ^= (const Region& rhs) {
    return xorSelf(rhs);
}
Region& Region::operator &= (const Region& rhs) {
    return andSelf(rhs);
}
Region& Region::operator -= (const Region& rhs) {
    return subtractSelf(rhs);
}
Region& Region::operator += (const Point& pt) {
    return translateSelf(pt.x, pt.y);
}

}; 

#endif 

