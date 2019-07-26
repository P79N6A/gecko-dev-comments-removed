















#ifndef ANDROID_UI_RECT
#define ANDROID_UI_RECT

#include <utils/TypeHelpers.h>
#include <ui/Point.h>

namespace android {

class Rect
{
public:
    int left;
    int top;
    int right;
    int bottom;

    typedef int value_type;

    
    

    inline Rect() {
    }
    inline Rect(int w, int h)
        : left(0), top(0), right(w), bottom(h) {
    }
    inline Rect(int l, int t, int r, int b)
        : left(l), top(t), right(r), bottom(b) {
    }
    inline Rect(const Point& lt, const Point& rb) 
        : left(lt.x), top(lt.y), right(rb.x), bottom(rb.y) {
    }

    void makeInvalid();

    inline void clear() {
        left = top = right = bottom = 0;
    }

    
    inline bool isValid() const {
        return (width()>=0) && (height()>=0);
    }

    
    inline bool isEmpty() const {
        return (width()<=0) || (height()<=0);
    }

    inline void set(const Rect& rhs) {
        operator = (rhs);
    }

    
    inline int width() const {
        return right-left;
    }
    
    
    inline int height() const {
        return bottom-top;
    }

    void setLeftTop(const Point& lt) {
        left = lt.x;
        top  = lt.y;
    }

    void setRightBottom(const Point& rb) {
        right = rb.x;
        bottom  = rb.y;
    }
    
    
    Point leftTop() const {
        return Point(left, top);
    }
    Point rightBottom() const {
        return Point(right, bottom);
    }
    Point rightTop() const {
        return Point(right, top);
    }
    Point leftBottom() const {
        return Point(left, bottom);
    }

    
    inline bool operator == (const Rect& rhs) const {
        return (left == rhs.left) && (top == rhs.top) &&
               (right == rhs.right) && (bottom == rhs.bottom);
    }

    inline bool operator != (const Rect& rhs) const {
        return !operator == (rhs);
    }

    
    
    bool operator < (const Rect& rhs) const;

    Rect& offsetToOrigin() {
        right -= left;
        bottom -= top;
        left = top = 0;
        return *this;
    }
    Rect& offsetTo(const Point& p) {
        return offsetTo(p.x, p.y);
    }
    Rect& offsetBy(const Point& dp) {
        return offsetBy(dp.x, dp.y);
    }
    Rect& operator += (const Point& rhs) {
        return offsetBy(rhs.x, rhs.y);
    }
    Rect& operator -= (const Point& rhs) {
        return offsetBy(-rhs.x, -rhs.y);
    }
    const Rect operator + (const Point& rhs) const;
    const Rect operator - (const Point& rhs) const;

    void translate(int dx, int dy) { 
        offsetBy(dx, dy);
    }
 
    Rect&   offsetTo(int x, int y);
    Rect&   offsetBy(int x, int y);
    bool    intersect(const Rect& with, Rect* result) const;
};

ANDROID_BASIC_TYPES_TRAITS(Rect)

}; 

#endif 
