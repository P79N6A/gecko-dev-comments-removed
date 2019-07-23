




package com.netscape.javascript.qa.drivers;

import java.io.*;

import java.applet.Applet;
import java.util.Vector;
import netscape.security.PrivilegeManager;
import netscape.javascript.JSObject;
import com.netscape.javascript.qa.liveconnect.LiveConnectTest;
import com.netscape.javascript.qa.liveconnect.jsobject.JSObject_001;













public class LiveNavEnv extends NavEnv {
    TestFile file;
    TestSuite suite;
    LiveNavDrv driver;
    private JSObject result;
    JSObject opener;
    JSObject testcases;
    JSObject location;

    boolean evaluatedSuccessfully = false;
    JSObject window;
    
    Applet applet;

    private String   WINDOW_NAME;

    







    public LiveNavEnv( TestFile file, TestSuite suite, LiveNavDrv driver ) {
        super( file, suite, driver );
    }
    
    






    public Object executeTestFile() {
        try {
            location = (JSObject) window.getMember( "location" );
            driver.p( file.name );

            
            

            String classname  = (file.name.substring(0, file.name.length() -
                ".class".length()) + ".html" );
            
            String s = driver.HTTP_PATH + classname; 
            
            System.out.println( "trying to set browser window to " + s );

            location.setMember( "href", s );
            evaluatedSuccessfully = waitForCompletion();

        } catch ( Exception e ) {
            driver.p( file.name + " failed with exception: " + e );
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
    
    














     public Applet getApplet() {
        Object document   = (Object) window.getMember("document");            
        Object applets    = ((JSObject) document).getMember("applets");
        return  (Applet) ((JSObject) applets).getSlot( 0 );
     }  
     
     




     
     public void getAppletClass(Applet applet) {
        try {
            driver.p( "the class of applet is " + 
                applet.getClass().toString() );

            driver.p( "is it a JSObject_001? " + 
                (applet instanceof JSObject_001 ));

            driver.p( "is it a LiveConnectTest? " + 
                (applet instanceof LiveConnectTest ));

            driver.p( "is it an Applet? " + 
                (applet instanceof Applet ));

            driver.p( "Try to cast applet to JSObject_001" );

            
                driver.p( ((JSObject_001) applet).toString() );

        } catch ( Exception e ) {
            driver.p( "parseResult threw exception: " + e );
        }
     }        

    












    public boolean parseResult() {
        applet = getApplet();
        
        Vector label = null;
        Vector value = null;
        
        try {
            FileReader fr = new FileReader(
                driver.OUTPUT_DIRECTORY.getAbsolutePath()+ 
                (driver.OUTPUT_DIRECTORY.getAbsolutePath().endsWith(File.separator) 
                ? ""
                : File.separator ) +
                LiveConnectTest.TEMP_LOG_NAME);
            BufferedReader br = new BufferedReader( fr );
            String classname  = br.readLine();
            boolean passed    = (new Boolean(br.readLine())).booleanValue();
            int length        = (new Double(br.readLine())).intValue();
            int no_passed     = (new Double(br.readLine())).intValue();
            int no_failed     = (new Double(br.readLine())).intValue();
            if ( ! passed ) {
                this.file.passed = false;
                this.suite.passed = false;
            }
            this.file.totalCases   += length;
            
            this.file.casesPassed  += no_passed;
            this.suite.casesPassed += no_passed;
            
            this.file.casesFailed  += no_failed;
            this.suite.casesFailed += no_failed;

        } catch ( IOException e ) {
            driver.p( e.toString() );
            return false;
        }
        return true;
    }        
    
    



    public void close() {
        applet.destroy();
        opener.eval( WINDOW_NAME +".close()" );
        opener.eval( "delete " + WINDOW_NAME );
    }
}
