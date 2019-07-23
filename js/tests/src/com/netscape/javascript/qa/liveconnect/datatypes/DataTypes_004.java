
package com.netscape.javascript.qa.liveconnect.datatypes;

import com.netscape.javascript.qa.liveconnect.*;











public class DataTypes_004 extends LiveConnectTest {
    public DataTypes_004() {
        super();
    }

    public static void main( String[] args ) {
        DataTypes_004 test = new DataTypes_004();
        test.start();
    }

    public void setupTestEnvironment() {
        super.setupTestEnvironment();
        global.eval( "var DT = "+
            "Packages.com.netscape.javascript.qa.liveconnect.DataTypeClass"
        );
    }

    











    public void executeTest() {
        getJSVarWithEval( "null", "netscape.javascript.JSObject", "null" );
        getJSVarWithGetMember( "null", "netscape.javascript.JSObject", "null" );
    }

    











    public void getJSVarWithEval( String rightExpr, String className,
        String value ) {

        String varName = "jsVar";
        Object result;
        Class expectedClass = null;

        try {
            expectedClass = Class.forName( className );
            
            global.eval( "var " + varName +" = " + rightExpr +";" );

            
            result = global.eval( varName );
            
            System.out.println( "result is " + result );

        } catch ( Exception e ) {
            System.err.println( "setJSVarWithEval threw " + e.toString() +
                " with arguments ( " + rightExpr +", "+
                expectedClass.getName() +", "+ value.toString() );

            e.printStackTrace();

            exception = e.toString();
            result = new Object();
        }

        try {
            addTestCase( 
                "global.eval( \"var "+ varName +" = " + rightExpr +"); "+
                "result = global.eval( "+ varName +"); " +
                "(result == null)",
                "true",
                (result == null) + "",
                exception );

        } catch ( Exception e ) {
            file.exception = e.toString();
        }

    }
    public void getJSVarWithGetMember( String rightExpr, String className,
        String value ) {

        String varName = "jsVar";
        Object result;
        Class expectedClass = null;

        try {
            expectedClass = Class.forName( className );
            
            global.eval( "var " + varName +" = " + rightExpr +";" );

            
            result = global.getMember( varName );
        } catch ( Exception e ) {
            System.err.println( "setJSVarWithGetMember threw " + e.toString() +
                " with arguments ( " + rightExpr +", "+
                expectedClass.getName() +", "+ value.toString() );
            e.printStackTrace();
            exception = e.toString();
            result = new Object();
        }

        try {
            addTestCase( 
                "global.eval( \"var "+ varName +" = " + rightExpr +"); "+
                "result = global.getMember( "+ varName +"); " +
                "(result == null)",
                "true",
                (result == null) + "",
                exception );

        } catch ( Exception e ) {
            file.exception = e.toString();
        }
    }        
 }