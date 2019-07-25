

























#pragma once

namespace graphite2 {

class Position
{
public:
    Position() : x(0), y(0) { }
    Position(const float inx, const float iny) { x = inx; y = iny; }
    Position operator + (const Position& a) const { return Position(x + a.x, y + a.y); }
    Position operator - (const Position& a) const { return Position(x - a.x, y - a.y); }
    Position operator * (const float m) const { return Position(x * m, y * m); }
    Position &operator += (const Position &a) { x += a.x; y += a.y; return *this; }
    Position &operator *= (const float m) { x *= m; y *= m; return *this; }

    float x;
    float y;
};

} 
