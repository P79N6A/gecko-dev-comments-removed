




package com.netscape.javascript.qa.drivers;

import com.netscape.javascript.qa.liveconnect.LiveConnectTest;
import java.lang.*;
import java.io.*;
import java.util.Vector;





























public class LiveConnectEnv implements TestEnvironment {
    TestFile        file;
    TestSuite       suite;
    LiveConnectDrv  driver;
    ObservedTask    task;
    String          TEMP_LOG_NAME;
    File            helper;

    


    public LiveConnectEnv( TestFile f, TestSuite s, LiveConnectDrv d) {
        this.file = f;
        this.suite = s;
        this.driver = d;
        this.TEMP_LOG_NAME = "js" + getRandomFileName() +".tmp";
    }

    


    public synchronized void runTest() {
        try {
            createContext();
            file.startTime = driver.getCurrentTime();            
            executeTestFile();
            file.endTime = driver.getCurrentTime();

            if (task.getExitValue() != 0) {
                System.out.println( "Abmormal program termination.  "+
                    "Exit value: " + task.getExitValue() );
                if ( file.name.endsWith( "-n.js" )) {
                    file.passed = true;
                } else {
                    file.exception = "Process exit value: " + task.getExitValue();
                    suite.passed   = false;
                    file.passed    = false;
                }
            }
            parseResult();

        } catch ( Exception e ) {
            suite.passed = false;
            file.passed  = false;
            e.printStackTrace();                
        }        
    }
    
    



    public Object createContext() {
        task = new ObservedTask( driver.EXECUTABLE + "  " +
                driver.HELPER_FUNCTIONS.getAbsolutePath() + " " +
                file.filePath +" "+
                TEMP_LOG_NAME,
                this );
        return (Object) task;
    }

    


    public Object executeTestFile() {
        try {
            task.exec();
        }   catch ( IOException e ) {
            System.err.println( e );
            file.exception = e.toString();
        }
        return null;
    }

    












    public synchronized boolean parseResult() {
        String line;
        BufferedReader buf = new BufferedReader(new StringReader(
            new String(task.getInput())));
        try {
            do {
                line = buf.readLine();
                System.out.println( line );
            }  while( line != null ) ;              
        } catch ( IOException e ) {
            System.err.println( "Exception reading process output:" + 
                e.toString() );
            file.exception  = e.toString();
            return false;
        }            

        Vector label = null;
        Vector value = null;
        
        String t = null;
        
        try {
            FileReader fr = new FileReader(
                driver.OUTPUT_DIRECTORY.getAbsolutePath()+ 
                TEMP_LOG_NAME );
                
            BufferedReader br = new BufferedReader( fr );
            String classname  = br.readLine();
            boolean passed    = (new Boolean(br.readLine())).booleanValue();
            int length        = (new Double(br.readLine())).intValue();
            int no_passed     = (new Double(br.readLine())).intValue();
            int no_failed     = (new Double(br.readLine())).intValue();
            String bugnumber  = br.readLine();
            
            if ( ! passed ) {
                this.file.passed = false;
                this.suite.passed = false;
            }
            this.file.totalCases   += length;
            this.suite.totalCases  += length;
            this.file.casesPassed  += no_passed;
            this.suite.casesPassed += no_passed;
            this.file.casesFailed  += no_failed;
            this.suite.casesFailed += no_failed;
            this.file.bugnumber    = bugnumber;
            
        } catch ( IOException e ) {
            System.err.println( e );
            e.printStackTrace();
        }
        return true;
    }        

    public String getRandomFileName() {
        return (Integer.toString((new Double(Math.random()*100000)).intValue()));
    }

    



    public void close(){
        String templog = driver.OUTPUT_DIRECTORY + TEMP_LOG_NAME;
        try {
            File f = new File ( templog );            
            if ( f.exists() ) {
                f.delete();
            }
        } catch ( Exception e ) {
            e.printStackTrace();
        }
        return;
    }
    
    void p( String s ) {
        System.out.println( s );
    }        
    
}
