package com.netscape.javascript.qa.lc3.jsnull;
















public class Null_001 {
    public String BOOLEAN = "BOOLEAN";
    public String BOOLEAN_OBJECT = "BOOLEAN_OBJECT";
    public String OBJECT = "OBJECT";
    public String STRING = "STRING";
    public String LONG   = "LONG";
    public String INT    = "INT";
    public String SHORT  = "SHORT";
    public String CHAR   = "CHAR";
    public String BYTE   = "BYTE";
    public String DOUBLE = "DOUBLE";
    public String FLOAT  = "FLOAT";


    
    public String ambiguous( float arg ) {
        return FLOAT;
    }

    public String ambiguous( double arg ) {
        return DOUBLE;
    }

    public String ambiguous( byte arg ) {
        return BYTE;
    }

    public String ambiguous( char arg ) {
        return CHAR;
    }

    public String ambiguous( short arg ) {
        return SHORT;
    }

    public String ambiguous( int arg ) {
        return INT;
    }

    public String ambiguous( long arg ) {
        return LONG;
    }

    public String ambiguous( String arg ) {
        return STRING;
    }

    public String ambiguous( Object arg ) {
        return OBJECT;
    }

    public String ambiguous( Boolean arg ) {
        return BOOLEAN_OBJECT;
    }

    public static String STATIC_BOOLEAN_OBJECT = "STATIC_BOOLEAN_OBJECT";
    public static String STATIC_STRING  = "STATIC_STRING";
    public static String STATIC_OBJECT  = "STATIC_OBJECT";

    public static String staticAmbiguous( Boolean arg ) {
        return STATIC_BOOLEAN_OBJECT;
    }
    
    public static String staticAmbiguous( String arg ) {
        return STATIC_STRING;
    }

    public static String staticAmbiguous( Object arg ) {
        return STATIC_OBJECT;
    }
   
}