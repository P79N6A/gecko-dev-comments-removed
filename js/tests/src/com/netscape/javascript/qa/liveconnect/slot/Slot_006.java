package com.netscape.javascript.qa.liveconnect.slot;

import netscape.javascript.*;
import com.netscape.javascript.qa.liveconnect.*;






















public class Slot_006 extends LiveConnectTest {
    public Slot_006() {
        super();
    }
    public static void main( String[] args ) {
        Slot_006 test = new Slot_006();
        test.start();
    }

    public void executeTest() {
        Object testMatrix[] = getDataArray();

        for ( int i = 0; i < testMatrix.length; i++ ) {
                JSObject jsObject = getJSString( (Object[]) testMatrix[i] );
                
                for ( int j = 0; 
                    j < ((String) ((Object[]) testMatrix[i])[1]).length(); 
                    j++ ) 
                {
                    getSlot( jsObject, j, (Object[]) testMatrix[i] );
                }                    
        }
    }

    public JSObject getJSString ( Object[] data ) {
        return (JSObject) data[0];
    }

   






    public Object[] getDataArray() {
        Object item0[] = {
            global.eval("new String(\"passed!\")"),
            new String("passed!")
         };

        Object dataArray[] = { item0 };
        return dataArray;
    }

    












    public void getSlot( JSObject jsString, int slot, Object[] data ) {
        String exception = "";
        JSObject constructor = null;
        Class  eClass = null;
        Class  aClass = null;
        String result = null;        
        String eResult = null;

        try {
            eResult = new Character( ((String)data[1]).charAt(slot) ).toString();
            result = (String) jsString.getSlot( slot );
            if ( result != null ) {
                eClass = Class.forName( "java.lang.String" );
                aClass = result.getClass();
            }
        }  catch ( Exception e ) {
            exception = "jsString.getSlot( " + slot + " ) "+
                "threw " + e.toString();
            file.exception += exception;
            e.printStackTrace();
        } finally {
            if ( result == null ) {
                addTestCase(
                    "[jsString.getSlot(" + slot +").toString(). returned " + result +"] "+
                    "( " + eResult +".equals( " + result +" )",
                    "true",
                    eResult.equals( result +"") +"",
                    exception );

            } else {
                
                addTestCase(
                    "[jsString.getSlot(" + slot +") returned " + result +"] "+
                    eResult +".toString().equals( " + result +" ) ",
                    "true",
                    eResult.equals( result.toString() ) +"",
                    exception );

                
                addTestCase(
                    "( " + eClass +".equals( " + aClass +" ) )",
                    "true",
                    eClass.equals( aClass ) +"",
                    exception );
            }                    
        }
    }
 }
