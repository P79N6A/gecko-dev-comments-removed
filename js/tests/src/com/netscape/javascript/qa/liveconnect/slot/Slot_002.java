package com.netscape.javascript.qa.liveconnect.slot;

import netscape.javascript.*;
import com.netscape.javascript.qa.liveconnect.*;























public class Slot_002 extends LiveConnectTest {
    public Slot_002() {
        super();
    }
    public static void main( String[] args ) {
        Slot_002 test = new Slot_002();
        test.start();
    }

    public void executeTest() {
        getBaseObjects();
        Object testMatrix[] = getDataArray();
        String args = getArrayArguments( testMatrix );
        
        JSObject jsArray = createJSArray( args );
        getLength( jsArray, testMatrix.length );
        
        for ( int i = 0; i < testMatrix.length; i++ ) {
                getSlot( jsArray, i, (Object[]) testMatrix[i] );
        }
    }
    
    public String getArrayArguments( Object[] matrix ) {
        StringBuffer buffer = new StringBuffer();
        for ( int i = 0; i < matrix.length; i++ ) {
            buffer.append( ((Object[]) matrix[i]) [0] );
            if ( i < matrix.length -1 ) {
                buffer.append( ", " );
            }                
        }
        return buffer.toString();
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
            new String("new String(\"JavaScript String\")"),
            new String ("JavaScript String"),
            jsString };
            
        Object item1[] = {
                new String( "new Number(12345)" ),
                new String( "12345" ),
                jsNumber };
                
        Object item2[] = {     
            new String( "new Boolean(false)" ),
            new String( "false" ),
            jsBoolean };
            
        Object item3[] = {
            new String( "new Array(0,1,2,3,4)" ),
            new String( "0,1,2,3,4" ),
            jsArray };
            
        Object item4[] = {
            new String( "new Object()" ),
            new String( "[object Object]" ),
            jsObject };
            
        Object dataArray[] = { item0, item1, item2, item3, item4 };
        return dataArray;        
    }
    
    







    public JSObject createJSArray( String newArrayArguments ) {
        JSObject jsArray = null;
        String args = "var jsArray =  new Array( " + newArrayArguments +" )";
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
                "new Array(\""+newArrayArguments+"\"); " +
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
                    "( " + constructor +".equals( " + data[2] +" ) )",
                    "true",
                    constructor.equals( data[2] ) +"",
                    exception );
            }
        }
    }
 }

