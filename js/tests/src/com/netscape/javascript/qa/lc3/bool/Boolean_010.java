package com.netscape.javascript.qa.lc3.bool;




















public class Boolean_010 {
    public int BOOLEAN = 0;
    public int BOOLEAN_OBJECT = 1;
    public int OBJECT = 2;
    public int STRING = 4;
    public int LONG   = 8;
    public int INT    = 16;
    public int SHORT  = 32;
    public int CHAR   = 64;
    public int BYTE   = 128;
    public int DOUBLE = 256;
    public int FLOAT  = 512;

    public int ambiguous( float arg ) {
        return FLOAT;
    }

    public int ambiguous( double arg ) {
        return DOUBLE;
    }

}