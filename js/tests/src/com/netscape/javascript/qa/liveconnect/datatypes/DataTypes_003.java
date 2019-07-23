
package com.netscape.javascript.qa.liveconnect.datatypes;

import com.netscape.javascript.qa.liveconnect.*;


















public class DataTypes_003 extends LiveConnectTest {
    public DataTypes_003() {
        super();
    }

    public static void main( String[] args ) {
        DataTypes_003 test = new DataTypes_003();
        test.start();
    }

    public void setupTestEnvironment() {
        super.setupTestEnvironment();
        global.eval( "var DT = Packages.com.netscape.javascript.qa.liveconnect.DataTypeClass"  );
    }

    











    public void executeTest() {
        
        doGetJSVarTests( 
            "Packages.com.netscape.javascript.qa.liveconnect.DataTypeClass",
            "java.lang.Class",
            "class com.netscape.javascript.qa.liveconnect.DataTypeClass" );

        doGetJSVarTests( "new java.lang.String(\"java string\")",
            "java.lang.String",
            "java string" );

        
        
        doGetJSVarTests( "Boolean(true)", "java.lang.Boolean", "true" );
        doGetJSVarTests( "Boolean()", "java.lang.Boolean", "false" );
        doGetJSVarTests( "Boolean(false)", "java.lang.Boolean", "false" );        
        
        doGetJSVarTests( "Number(12345)",  "java.lang.Double",  new Double("12345").toString() );
        doGetJSVarTests( "12345",  "java.lang.Double",  new Double("12345").toString() );        
        
        doGetJSVarTests( "String(\"hello\")",  "java.lang.String", "hello" );
        doGetJSVarTests( "\"hello\"",  "java.lang.String", "hello" );
        
        doGetJSVarTests( "true", "java.lang.Boolean", "true" );        
        doGetJSVarTests( "false", "java.lang.Boolean", "false" );
        
        
        
        doGetJSVarTests( "new Number(0)",   "netscape.javascript.JSObject", "0" );






        doGetJSVarTests( "void 0",          "java.lang.String", "undefined" );

        doGetJSVarTests( "new Number(999)",   "netscape.javascript.JSObject",   "999" );
        
        doGetJSVarTests( "Math",    "netscape.javascript.JSObject",     "[object Math]" );
        
        doGetJSVarTests( "new Boolean(true)", "netscape.javascript.JSObject", "true" );        
        doGetJSVarTests( "new Boolean(false)", "netscape.javascript.JSObject", "false" );
        
        doGetJSVarTests( "new String()",   "netscape.javascript.JSObject", "" );
        doGetJSVarTests( "new String(\"hi\")",   "netscape.javascript.JSObject", "hi" );
        
        doGetJSVarTests( "new Array(1,2,3,4,5)", "netscape.javascript.JSObject", "1,2,3,4,5" );
        doGetJSVarTests( "[5,4,3,2,1]", "netscape.javascript.JSObject", "5,4,3,2,1" );
        
    }
    
    












    
    public void doGetJSVarTests( String rightExpr, String className,
        String value ) {
            getJSVarWithEval( rightExpr, className, value );
            getJSVarWithGetMember( rightExpr, className, value );
    }

    









    public void getJSVarWithEval( String rightExpr, String className,
        String value ) {

        String varName = "jsVar";
        Object jsObject;
        Object result;
        Class expectedClass = null;

        try {
            expectedClass = Class.forName( className );
            
            
            global.eval( "var " + varName +" = " + rightExpr +";" );

            
            jsObject = global.eval( varName );
        
        } catch ( Exception e ) {
            System.err.println( "setJSVarWithEval threw " + e.toString() +
                " with arguments ( " + rightExpr +", "+ 
                expectedClass.getName() +", "+ value.toString() );
            e.printStackTrace();                
            exception = e.toString();
            jsObject = new Object();
        }
        
        
        
        addTestCase( 
            "global.eval( \"var "+ varName +" = " + rightExpr +"); "+
            "jsObject = global.eval( "+ varName +"); " +
            "jsObject.getClass.getName().equals("+expectedClass.getName()+")"+
            "[ jsObject class is " + jsObject.getClass().getName() +"]",
            "true",
            jsObject.getClass().getName().equals(expectedClass.getName())+ "",
            exception );
        
        
        
        
        addTestCase(
            "("+jsObject.toString() +".equals(" + value.toString() +"))",
            "true",
            jsObject.toString().equals( value ) +"",
            exception );
    }

    








    public void getJSVarWithGetMember( String rightExpr, String className,
        String value ) {

        String varName = "jsVar";
        Object jsObject;
        Class expectedClass = null;

        try {
            expectedClass = Class.forName( className );
            
            
            global.eval( "var " + varName +" = " + rightExpr +";" );

            
            jsObject = global.getMember( varName );
            
        } catch ( Exception e ) {
            System.err.println( "getJSVarWithGetMember threw " + e.toString() +
                " with arguments ( " + rightExpr +", "+ 
                expectedClass.getName() +", "+ value.toString() );
            e.printStackTrace();                
            exception = e.toString();
            jsObject = new Object();
        }
        
        
        
        addTestCase( 
            "global.eval( \"var "+ varName +" = " + rightExpr +"); "+
                "jsObject = global.getMember( "+ varName +"); " +
                "jsObject.getClass.getName().equals(" + 
                expectedClass.getName() +")"+
                "[ jsObject class is "+jsObject.getClass().getName()+"]",
            "true",
            jsObject.getClass().getName().equals(expectedClass.getName()) +"",
            exception );

        

        addTestCase(
            "("+jsObject.toString() +".equals(" + value.toString() +"))",
            "true",
            jsObject.toString().equals( value ) +"",
            exception );
    }    
 }