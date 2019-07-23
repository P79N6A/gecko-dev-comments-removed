package com.netscape.javascript.qa.liveconnect.slot;

import netscape.javascript.*;
import com.netscape.javascript.qa.liveconnect.*;






















public class Slot_004 extends LiveConnectTest {
    public Slot_004() {
        super();
    }
    public static void main( String[] args ) {
        Slot_004 test = new Slot_004();
        test.start();
    }

    public void executeTest() {
        Object testMatrix[] = getDataArray();

        JSObject jsArray = createJSArray();
        getLength( jsArray, 0 );

        for ( int i = 0; i < testMatrix.length; i++ ) {
                setSlot( jsArray, i, (Object[]) testMatrix[i] );
                getSlot( jsArray, i, (Object[]) testMatrix[i] );
        }
        getLength( jsArray, testMatrix.length );
    }

   









    public Object[] getDataArray() {
        Object item0[] = {
            new String("Java String"),
            new String ("Java String"),
            "java.lang.String" };

        Object item1[] = {
            new Integer(12345),
            new String( "12345" ),
            "java.lang.Integer" };

        Object item2[] = {
            new Boolean(false),
            new String( "false" ),
            "java.lang.Boolean" };

        Object item3[] = {
            new Double(12345.0),
            new String( "12345.0" ),
            "java.lang.Double" };

        Object dataArray[] = { item0, item1, item2, item3 };
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
        Object result = null;
        Class  eClass = null;
        Class  aClass = null;

        try {
            result = (Object) jsArray.getSlot( slot );
            if ( result != null ) {
                eClass = Class.forName((String) data[2]);
                aClass = result.getClass();
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
                    "[jsArray.getSlot(" + slot +") returned " + result +"] "+
                    data[1] +".toString().equals( " + result +" ) ",
                    "true",
                    data[1].equals( result.toString() ) +"",
                    exception );

                
                addTestCase(
                    "( " + eClass +".equals( " + aClass +" ) )",
                    "true",
                    eClass.equals( aClass ) +"",
                    exception );
            }
        }
    }

    







    public void setSlot( JSObject jsArray, int slot, Object[] data ) {
        String exception = null;
        String result = "passed!";

        try {
            jsArray.setSlot( slot, (Object) data[0] );
        }  catch ( Exception e ) {
            result = "failed!";
            exception = "jsArray.setSlot( " + slot +","+ data[0] +") "+
                "threw " + e.toString();
            file.exception += exception;
            e.printStackTrace();
        } finally {
            addTestCase(
                "jsArray.setSlot( " + slot +", "+ data[0] +")",
                "passed!",
                result,
                exception );
        }
    }
 }
