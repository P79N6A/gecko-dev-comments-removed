




































package org.mozilla.fennec.gfx;

public class IntPoint {
    public final int x, y;

    public IntPoint(int inX, int inY) { x = inX; y = inY; }

    @Override
    public String toString() { return "(" + x + ", " + y + ")"; }

    
    public IntPoint add(IntPoint other) { return new IntPoint(x + other.x, y + other.y); }

    
    public IntPoint subtract(IntPoint other) { return new IntPoint(x - other.x, y - other.y); }

    
    public IntPoint scale(float scale) {
        return new IntPoint((int)Math.round((float)x * scale), (int)Math.round((float)y * scale));
    }
}


