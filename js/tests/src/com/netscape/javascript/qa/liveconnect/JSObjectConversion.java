


package com.netscape.javascript.qa.liveconnect;

import netscape.javascript.JSObject;
















public class JSObjectConversion {
    




    public static double PUB_DOUBLE_REPRESENTATION = 0.2134;

    public double doubleValue() {
        return PUB_DOUBLE_REPRESENTATION;
    }

    public int toNumber() {
        return PUB_NUMBER_REPRESENTATION;
    }

    



    public boolean booleanValue() {
        return PUB_BOOLEAN_REPRESENTATION;
    }

    public boolean PUB_BOOLEAN_REPRESENTATION = true;

    public static int PUB_NUMBER_REPRESENTATION = 2134;

    



    public String toString() {
        return PUB_STRING_REPRESENTATION;
    }

    public static String PUB_STRING_REPRESENTATION =
        "DataTypeClass Instance";

    public static JSObject staticGetJSObject() {
        return PUB_STATIC_JSOBJECT;
    }

    public JSObject getJSObject() {
        return PUB_JSOBJECT;
    }
    public static void staticSetJSObject(JSObject jso) {
        PUB_STATIC_JSOBJECT = jso;
    }
    public void setJSObject(JSObject jso) {
        PUB_JSOBJECT = jso;
    }

    public JSObject[] createJSObjectArray(int length) {
        PUB_JSOBJECT_ARRAY = new JSObject[length];
        return PUB_JSOBJECT_ARRAY;
    }

    public static JSObject[] staticCreateJSObjectArray(int length) {
        PUB_STATIC_JSOBJECT_ARRAY = new JSObject[length];
        return PUB_STATIC_JSOBJECT_ARRAY;
    }



    public static final JSObject PUB_STATIC_FINAL_JSOBJECT = null;
    public static JSObject       PUB_STATIC_JSOBJECT       = PUB_STATIC_FINAL_JSOBJECT;
    public JSObject              PUB_JSOBJECT              = PUB_STATIC_FINAL_JSOBJECT;
    
    public JSObject[] PUB_JSOBJECT_ARRAY;
    public static JSObject[] PUB_STATIC_JSOBJECT_ARRAY;
}