



package com.netscape.javascript.qa.liveconnect;

import java.applet.Applet;
import java.io.File;
import java.util.Vector;
import netscape.javascript.*;


import com.netscape.javascript.qa.drivers.*;

















































public class LiveConnectTest extends Applet implements Runnable {
    


    public LiveConnectTest() {
    }

    


    public static void main( String[] args ) {
        LiveConnectTest test = new LiveConnectTest();
    }
    
    public void start() {
        run();
    }        
    
    public void stop() {
    }
    
    



     
    public void run() {
        System.out.println("Running the test." );
        setupTestEnvironment();
        file.startTime = TestDriver.getCurrentTime();
        executeTest();
        file.endTime= TestDriver.getCurrentTime();
        getResults();
        cleanupTestEnvironment();
        stop();
    }
    
    


    public void setupTestEnvironment() {
        global  = JSObject.getWindow( this );    
      
        getEnvironment();
      
        if ( ENVIRONMENT == BROWSER ) {


        }            
        output  = getOutputDirectory();
        if (LOGGING) {
            TestDriver.openLogFiles( output );
            templog = getTempLog( output );
        }            
        testdir = getTestDirectory();
        file   = new TestFile( this.getClass().getName(), testdir.toString() +
            this.getClass().toString() );
            
        file.bugnumber = this.BUGNUMBER;            
        file.description = ( this.getClass().toString() );
    }
    
    public void getEnvironment() {
        this.ENVIRONMENT = ( global.getMember("version").equals( "undefined")) 
                    ? BROWSER
                      : SHELL;
        return;                      
    }        
    
    




    public TestLog getTempLog( File output ) {
        String templog = "";
        
        try {
            TEMP_LOG_NAME = ((String) global.getMember( "OUTPUT_FILE" )).equals
                ("undefined") 
                ? TEMP_LOG_NAME 
                : (String) global.getMember("OUTPUT_FILE");
       
            templog = output.getAbsolutePath() + TEMP_LOG_NAME;
        } catch ( Exception e ) {
            this.exception = e.toString();
            p("Exception deleting existing templog: " + e.toString() );
        }
        
        return new TestLog( templog, "" );
    }        

    








    public File getOutputDirectory() {
        String o = "";
        
        if ( this.ENVIRONMENT == BROWSER ) {
            String outputParam = this.getParameter( "output" );
            
            return new File( getParameter( "output" ) );
        } else {
            try {
                o = (String) global.getMember( "OUTPUT_DIRECTORY" );
                
                if ( ! o.equals( "undefined") ) {
                    LOGGING = true;                    
                }                    
            } catch ( Exception e ) {
                System.err.println( "OUTPUT_DIRECTORY threw: " + e );
                e.printStackTrace();
                System.exit(1);
            }
            
            System.out.println( "Output file is " + o.toString() );
            
            return new File( o.toString() );            
        }            
    }        
    
    





    public File getTestDirectory() {
        try {
            String o = (String) global.getMember( "TEST_DIRECTORY" );
            o = o.endsWith( File.separator ) ? o : o + File.separator;
            return new File( o.toString() );
        } catch ( Exception e ) {
            System.err.println( "TEST_DIRECTORY is not defined: " + e );
            return new File( "." );
        }        
    }        

    





    public void executeTest() {
        return;
    }
    
    









    
    public void addTestCase( String description, String expect, String actual,
        String exception ) 
    {
        String result = ( expect == actual ) ? "true" : "false";
        TestCase tc = new TestCase( result, this.getClass().getName().toString(), 
            description, expect, actual, exception );
        file.caseVector.addElement( tc );            
        file.totalCases++;        
        if ( result == "false" ) {
            file.passed = false;
            file.casesFailed++;
         } else {
            file.casesPassed++;
         }
        return;            
    }
    
    



    public void closeLogs() {
        TestDriver.getLog( output, TestDriver.SUMMARY_LOG_NAME ).closeLog();
        TestDriver.getLog( output, TestDriver.SUITE_LOG_NAME ).closeLog();
        TestDriver.getLog( output, TestDriver.FILE_LOG_NAME ).closeLog();
        TestDriver.getLog( output, TestDriver.CASE_LOG_NAME ).closeLog();
        templog.closeLog();
        templog = null;
    }        
    
    



    public void getResults() {
        displayResults();
        if (LOGGING) {
            writeResultsToCaseLog();
            writeResultsToFileLog();
            writeResultsToTempLog();
        }
    }
    
    



    public void displayResults() {
        for ( int i = 0; i < file.caseVector.size(); i++ ) {
            TestCase tc = (TestCase) file.caseVector.elementAt(i);
            p( tc.description +" = "+ tc.actual+ 
                ( tc.expect != tc.actual 
                ?  " FAILED!  expected: " + tc.expect
                :  " PASSED!" ) );
        }
        getFailedCases();
    }        
    
    





    public void writeResultsToCaseLog() {
        if ( !file.passed ) {
            TestDriver.writeCaseResults( file, file.description, output );
        }            
    }        
    
    





    public void writeResultsToFileLog() {
        if ( !file.passed ) {
            TestDriver.writeFileResult(file, null, output);
        }            
    }     
    
    

























    public void writeResultsToTempLog(){
        System.out.println( "Writing results to " + templog.toString() );
        
        templog.writeLine( file.description );
        templog.writeLine( file.passed + "" );
        templog.writeLine( file.caseVector.size() +"" );
        templog.writeLine( file.casesPassed + "");
        templog.writeLine( file.casesFailed + "");
        templog.writeLine( file.bugnumber );
        
        p( file.name );
        p( "passed:       " + passed );
        p( "total cases:  " + file.caseVector.size() );
        p( "cases passed: " + file.casesPassed );
        p( "cases failed: " + file.casesFailed );
        p( "bugnumber:    " + file.bugnumber );
        return;       
    }        

    




    public void cleanupTestEnvironment() {
        try {
            if ( LOGGING ) {

            }                

        } catch ( Exception e ) {
            p( "exception in cleanupTestEnvironment: " + e.toString() );
            e.printStackTrace();
        }            
    }        
    
    public void p (String s ) {
        System.out.println( s );
    }        
    
    






    public static String catchException(JSObject self, String method,
                                        Object args[]) {
        Object rval;
        String msg;
        try {
            rval = self.call(method, args);
            msg = NO_EXCEPTION;
        } catch (Throwable e) {
            msg = e.getMessage();
        }
        return msg;
    }
    
    public void getFailedCases() {
        if ( file.passed ) {
            return;
        }
        
        p( "********** Failed Test Cases **********" );            
        
        for ( int i = 0; i < file.caseVector.size(); i++ ) {
            TestCase tc = (TestCase) file.caseVector.elementAt(i);
            
            if ( tc.passed != "true" ) {
                
                p( tc.description +" = "+ tc.actual+ 
                    ( tc.expect != tc.actual 
                    ?  " FAILED!  expected: " + tc.expect
                    :  " PASSED!" 
                    ) +
                    ( tc.reason.length() > 0
                    ?  ": " + tc.reason
                    :  ""
                    )
                );
            }                    
        }
    }        
    
    public File output;
    public File testdir;
    
    public TestFile file;
    
    public Vector testcase;
    public JSObject global;
    
    public boolean  passed = true;
    public Vector   failedVector = new Vector();
    public TestLog  templog;
    public String   exception = "";
    
    public String BUGNUMBER = "";    
    
    boolean LOGGING = false;
    public static String TEMP_LOG_NAME = "result.tmp";
        
    public static final int BROWSER = 1;
    public static final int SHELL   = 0;
    
    public int ENVIRONMENT;
    
    public static final String NO_EXCEPTION = "Expected exception not thrown.";
    
    public static final String DATA_CLASS = "com.netscape.javascript.qa.liveconnect.DataTypeClass";
}
