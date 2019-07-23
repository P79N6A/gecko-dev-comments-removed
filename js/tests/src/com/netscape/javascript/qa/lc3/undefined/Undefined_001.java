package com.netscape.javascript.qa.lc3.undefined;
















public class Undefined_001 {
    public String OBJECT = "OBJECT";
    public String STRING = "STRING";

    public static String STATIC_OBJECT = "OBJECT";
    public static String STATIC_STRING = "STRING";


    public String ambiguous( String arg ) {
        return STRING;
    }

    public String ambiguous( Object arg ) {
        return OBJECT;
    }

    public static String staticAmbiguous( String arg ) {
        return STATIC_STRING;
    }

    public static String staticAmbiguous( Object arg ) {
        return STATIC_OBJECT;
    }

}