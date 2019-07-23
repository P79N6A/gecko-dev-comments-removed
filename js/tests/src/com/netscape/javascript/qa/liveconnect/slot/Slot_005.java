package com.netscape.javascript.qa.liveconnect.slot;

import netscape.javascript.*;
import com.netscape.javascript.qa.liveconnect.*;






















public class Slot_005 extends LiveConnectTest {
    public Slot_005() {
        super();
    }
    public static void main( String[] args ) {
        Slot_005 test = new Slot_005();
        test.start();
    }

    public void executeTest() {
        getBaseObjects();
        Object testMatrix[] = getDataArray();

        for ( int i = 0; i < testMatrix.length; i++ ) {
                JSObject jsObject = createJSObject((Object[]) testMatrix[i]);
                setSlot( jsObject, i, (Object[]) testMatrix[i] );
                getSlot( jsObject, i, (Object[]) testMatrix[i] );
        }
    }

    public JSObject createJSObject( Object[] data ) {
        return (JSObject) data[0];
    }

    public JSObject jsNumber;
    public JSObject jsFunction;
    public JSObject jsBoolean;
    public JSObject jsObject;
    public JSObject jsString;
    public JSObject jsMath;
    public JSObject jsDate;
    public JSObject jsArray;
    public JSObject jsRegExp;

    





    public boolean getBaseObjects() {
        try {
            jsNumber   = (JSObject) global.eval( "Number.prototype.constructor" );
            jsString   = (JSObject) global.eval( "String.prototype.constructor" );
            jsFunction = (JSObject) global.eval( "Function.prototype.constructor" );
            jsBoolean  = (JSObject) global.eval( "Boolean.prototype.constructor" );
            jsObject   = (JSObject) global.eval( "Object.prototype.constructor" );
            jsMath     = (JSObject) global.eval("Math");
            jsDate     = (JSObject) global.eval( "Date.prototype.constructor" );
            jsArray    = (JSObject) global.eval( "Array.prototype.constructor" );
            jsRegExp   = (JSObject) global.eval("RegExp.prototype.constructor");

        } catch ( Exception e ) {
            System.err.println( "Failed in getBaseObjects:  " + e.toString() );
            e.printStackTrace();
            return false;
        }

        return true;
    }

   









    public Object[] getDataArray() {
        Object item0[] = {
            global.eval("new String(\"passed!\")"),
            global.eval("new String(\"passed!\")"),
            new String("passed!"),
            jsString
         };

        Object item1[] = {
            global.eval("new Number(12345)"),
            global.eval( "new Number(98765)"),
            new String("98765"),
            jsNumber};

        Object item2[] = {
            global.eval("new Boolean(false)"),
            global.eval("new Boolean(true)"),
            new String("true"),
            jsBoolean };

        Object item3[] = {
            global.eval("new Array(0,1,2,3,4)"),
            global.eval("new Array(\"h\",\"i\")"),
            new String( "h,i" ),
            jsArray };

        Object item4[] = {
            global.eval("new Object()"),
            global.eval("new Object()"),
            new String( "[object Object]" ),
            jsObject };

        Object dataArray[] = { item0, item1, item2, item3, item4 };
        return dataArray;
    }

    




    public JSObject createJSArray() {
        JSObject jsArray = null;

        String args = "var jsArray =  new Array()";
        String result = "passed!";
        try {
            System.out.println( args );
            global.eval( args );
            jsArray = (JSObject) global.getMember("jsArray");

        } catch ( Exception e ) {
            result = "failed!";
            exception = "global.getMember(\"jsArray\")"+
                "threw " + e.toString();
            file.exception += exception;
            e.printStackTrace();
        } finally {
            addTestCase(
                "global.eval(\"var jsArray = "+
                "new Array(); " +
                "JSObject jsArray = (JSObject) global.getMember(\"jsArray\")",
                "passed!",
                result,
                exception );
        }
        return jsArray;
    }

    





    public void getLength( JSObject jsArray, int javaLength ) {
        String exception = "";
        int jsLength = 0;

        try {
            jsLength = ((Double) jsArray.getMember("length")).intValue();
        }  catch ( Exception e ) {
            exception = "jsArray.getMember(\"length\") threw "+
                e.toString();
            file.exception += exception;
            e.printStackTrace();
        } finally {
            addTestCase(
                "[length is " + jsLength +"] "+
                "( jsArray.getMember(\"length\")).intValue() == " +
                javaLength +" )",
                "true",
                (jsLength == javaLength) + "",
                exception );
        }
    }

    










    public void getSlot( JSObject jsArray, int slot, Object[] data ) {
        String exception = "";
        JSObject constructor = null;
        JSObject result = null;
        Class  eClass = null;
        Class  aClass = null;

        try {
            result = (JSObject) jsArray.getSlot( slot );
            if ( result != null ) {
                eClass = Class.forName( "netscape.javascript.JSObject" );
                aClass = result.getClass();
                constructor = (JSObject) result.getMember( "constructor" );
            }
        }  catch ( Exception e ) {
            exception = "jsArray.getSlot( " + slot + " ) "+
                "threw " + e.toString();
            file.exception += exception;
            e.printStackTrace();
        } finally {
            if ( result == null ) {
                addTestCase(
                    "[jsArray.getSlot(" + slot +").toString().trim() returned " + result +"] "+
                    "( " + data[1] +".equals( " + result +" )",
                    "true",
                    data[1].toString().trim().equals( result.toString() ) +"",
                    exception );

            } else {









                
                addTestCase(
                    "( " + eClass +".equals( " + aClass +" ) )",
                    "true",
                    eClass.equals( aClass ) +"",
                    exception );

                
                addTestCase(
                    "( " + constructor +".equals( " + data[3] +" ) )",
                    "true",
                    constructor.equals( data[3] ) +"",
                    exception );
            }
        }
    }

    







    public void setSlot( JSObject jsArray, int slot, Object[] data ) {
        String exception = null;
        String result = "passed!";

        try {
            jsArray.setSlot( slot, (Object) data[1] );
        }  catch ( Exception e ) {
            result = "failed!";
            exception = "jsArray.setSlot( " + slot +","+ data[1] +") "+
                "threw " + e.toString();
            file.exception += exception;
            e.printStackTrace();
        } finally {
            addTestCase(
                "jsArray.setSlot( " + slot +", "+ data[1] +")",
                "passed!",
                result,
                exception );
        }
    }
 }
