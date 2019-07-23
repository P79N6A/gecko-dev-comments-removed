package com.netscape.javascript.qa.liveconnect.slot;

import netscape.javascript.*;
import com.netscape.javascript.qa.liveconnect.*;














































public class Slot_001 extends LiveConnectTest {
    public Slot_001() {
        super();
    }
    public static void main( String[] args ) {
        Slot_001 test = new Slot_001();
        test.start();
    }

    
    
    
    
    Object test1[] = { new String("null"), null };
    Object test2[] = { new String( "" ) };
    Object test3[] = { new String("true,\"hi\",3.14159"), 
        new Boolean(true), new String("hi"), new Double("3.14159") };
        
    Object test4[] = { new String( "new java.lang.Boolean(true), " +
        "new java.lang.String(\"hello\"), java.lang.System.out, "+
        "new java.lang.Integer(5)"),
        new Boolean(true),
        new String("hello"),
        System.out,
        new Integer(5) };
    
    Object testMatrix[] =  { test1, test2, test3, test4 };
    
    public void executeTest() {
        for ( int i = 0; i < testMatrix.length; i++ ) {
            String args = (String) ((Object[]) testMatrix[i])[0];
            
            JSObject jsArray = createJSArray( args );
            
            
            int javaLength = ((Object[]) testMatrix[i]).length -1;
            
            getLength( jsArray, javaLength );

            for ( int j = 0; j < javaLength ; j++ ) {
                getSlot( jsArray, j, ((Object[]) testMatrix[i])[j+1] );
            }                
        }            
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
    
    







    public void getSlot( JSObject jsArray, int slot, Object javaValue ) {
        String exception = "";
        Object result = null;
        Class  eClass = null;
        Class  aClass = null;

        try {
            result = jsArray.getSlot( slot );
            if ( javaValue != null ) {
                eClass = javaValue.getClass();
                aClass = result.getClass();
            }                
        }  catch ( Exception e ) {
            exception = "jsArray.getSlot( " + slot + " ) "+
                "threw " + e.toString();
            file.exception += exception;
            e.printStackTrace();
        } finally {
            if ( javaValue == null ) {
                addTestCase(
                    "[jsArray.getSlot(" + slot +") returned " + result +"] "+
                    "( " +javaValue +" == " + result +" )",
                    "true",
                    (javaValue == result ) +"",
                    exception );
                
            } else {                
                addTestCase(
                    "[jsArray.getSlot(" + slot +") returned " + result +"] "+
                    javaValue +".equals( " + result +" ) ",
                    "true",
                    javaValue.equals( result ) +"",
                    exception );
                    
                addTestCase( 
                    "( " + eClass +".equals( " + aClass +" ) )",
                    "true",
                    eClass.equals( aClass ) +"",
                    exception );
            }                    
        }
    }
    
    







    public void setSlot( JSObject jsArray, int slot, Object javaValue ) {
        String exception = null;
        String result = "passed!";

        try {
            jsArray.setSlot( slot, javaValue );
        }  catch ( Exception e ) {
            result = "failed!";
            exception = "jsArray.setSlot( " + slot +","+ javaValue +") "+
                "threw " + e.toString();
            file.exception += exception;
            e.printStackTrace();
        } finally {
            addTestCase(
                "jsArray.setSlot( " + slot +", "+ javaValue +")",
                "passed!",
                result,
                exception );
        }
    }
 }

