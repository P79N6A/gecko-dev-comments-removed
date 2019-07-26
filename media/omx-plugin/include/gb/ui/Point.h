















#ifndef ANDROID_UI_POINT
#define ANDROID_UI_POINT

#include <utils/TypeHelpers.h>

namespace android {

class Point
{
public:
    int x;
    int y;

    
    

    
    inline Point() {
    }
    inline Point(int x, int y) : x(x), y(y) {
    }

    inline bool operator == (const Point& rhs) const {
        return (x == rhs.x) && (y == rhs.y);
    }
    inline bool operator != (const Point& rhs) const {
        return !operator == (rhs);
    }

    inline bool isOrigin() const {
        return !(x|y);
    }

    
    
    bool operator < (const Point& rhs) const {
        return y<rhs.y || (y==rhs.y && x<rhs.x);
    }

    inline Point& operator - () {
        x = -x;
        y = -y;
        return *this;
    }
    
    inline Point& operator += (const Point& rhs) {
        x += rhs.x;
        y += rhs.y;
        return *this;
    }
    inline Point& operator -= (const Point& rhs) {
        x -= rhs.x;
        y -= rhs.y;
        return *this;
    }
    
    const Point operator + (const Point& rhs) const {
        const Point result(x+rhs.x, y+rhs.y);
        return result;
    }
    const Point operator - (const Point& rhs) const {
        const Point result(x-rhs.x, y-rhs.y);
        return result;
    }    
};

ANDROID_BASIC_TYPES_TRAITS(Point)

}; 

#endif 
