




package com.netscape.javascript.qa.drivers;

import java.io.*;

import netscape.javascript.JSObject;
import com.netscape.javascript.qa.drivers.*;












public class NavEnv implements TestEnvironment {
    TestFile file;
    TestSuite suite;
    NavDrv driver;
    private JSObject result;
    JSObject opener;
    JSObject testcases;
    JSObject location;

    boolean evaluatedSuccessfully = false;
    JSObject window;
   
    private String   WINDOW_NAME;
    
    


    public NavEnv( TestFile f, TestSuite s, NavDrv d ) {
        this.file = f;
        this.suite = s;
        this.driver =d;

        this.opener = (JSObject) JSObject.getWindow( d );
        this.window = null;
        this.WINDOW_NAME = "js" + getRandomWindowName();
    }
    


    public synchronized void runTest() {
        int i = 0;
        System.out.println( file.name );
        try {
            createContext();
            file.startTime = driver.getCurrentTime();
            System.out.println( i++ );
            executeTestFile();
            System.out.println( i++ );
            
            if ( evaluatedSuccessfully ) {
            System.out.println( i++ );
                
                parseResult();
            System.out.println( i++ );
                
            }
            file.endTime = driver.getCurrentTime();            

        } catch ( Exception e ) {
            suite.passed = false;
            file.passed = false;
            file.exception = "file failed with exception: " + e ;
        }
    }

    



    public Object createContext() {
        System.out.println( "opening window" );
        opener.eval( WINDOW_NAME +" = window.open( '', '" + WINDOW_NAME + "' )" );
        window = (JSObject) opener.getMember( WINDOW_NAME );
        return window;
    }

    




    public Object executeTestFile() {
        System.out.println( "executeTestFile()" );
        try {
            location = (JSObject) window.getMember( "location" );
            System.out.println( file.name );
            String s = driver.HTTP_PATH + suite.name +"/" + file.name;
            location.setMember( "href", driver.HTTP_PATH + suite.name +"/" + 
                file.name );
            evaluatedSuccessfully = waitForCompletion();

        } catch ( Exception e ) {
            System.err.println( file.name + " failed with exception: " + e );
            file.exception = e.toString();            
            if ( file.name.endsWith( "-n.js" ) ) {
                this.file.passed = true;
                evaluatedSuccessfully = true;
            } else {
                this.file.passed = false;
                this.suite.passed = false;
                evaluatedSuccessfully = false;                
            }
        }         
        return null;
    }
    
    










    public boolean waitForCompletion() {
        int counter = 0;
        if ( ! window.getMember( "completed" ).toString().equals("true") ) {
            while (!window.getMember("completed").toString().equals("true")) {
                try {
                    if ( counter > 20 ) {
                        file.passed = false;
                        file.exception += "test failed to complete";
                        System.out.println( "test failed to complete" );
                        return false;
                    }
                    System.out.println(".");
                    driver.sleep( 1000 );
                    counter++;
                } catch ( Exception e ) {
                    System.out.println( "sleep failed:  " + e );
                    return false;
                }
            }
        }
        return true;
    }

    




    public synchronized boolean parseResult() {
        try {
           JSObject testcases = (JSObject) window.getMember("testcases");
            file.totalCases = ((Number) ((JSObject) testcases).getMember("length")).intValue();
            System.out.println( "testcases.length is " + file.totalCases );
            for ( int i = 0; i < file.totalCases; i++ ) {
                JSObject tc  = (JSObject) ((JSObject) testcases).getSlot(i);

                TestCase nc = new TestCase(
                    tc.getMember("passed") == null ? "null" : tc.getMember("passed").toString(),
                    tc.getMember("name") == null ? "null " : tc.getMember("name").toString(),
                    tc.getMember("description") == null ? "null " : tc.getMember("description").toString(),
                    tc.getMember("expect") == null ? "null " : tc.getMember("expect").toString(),
                    tc.getMember("actual") == null ? "null " : tc.getMember("actual").toString(),
                    tc.getMember("reason") == null ? "null " : tc.getMember("reason").toString()
                );

                file.caseVector.addElement( nc );

                if ( nc.passed.equals("false") ) {
                    if ( file.name.endsWith( "-n.js" ) ) {
                        this.file.passed = true;
                    } else {
                        this.file.passed = false;
                        this.suite.passed = false;
                    }
                }
            }
        } catch ( Exception e ) {
            System.out.println( e );
             file.exception = e.toString();
            if ( file.name.endsWith( "-n.html" ) ) {
                file.passed = true;
            } else {
                file.passed = false;
                suite.passed = false;
            }
        }

        
        if ( this.file.passed == false ) {
            try {
                this.file.bugnumber = window.getMember("BUGNUMBER").toString();
            }   catch ( Exception e ) {
                
            }                
        }            
        
        return true;
    }
    
    


    public void close() {
        opener.eval( WINDOW_NAME +".close()" );
        opener.eval( "delete " + WINDOW_NAME );
    }
    



    public String getRandomWindowName() {
        return (Integer.toString((new Double(Math.random()*100000)).intValue()));
    }
}