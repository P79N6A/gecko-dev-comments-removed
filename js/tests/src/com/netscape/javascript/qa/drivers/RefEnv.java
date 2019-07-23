




package com.netscape.javascript.qa.drivers;

import java.io.*;
import java.util.StringTokenizer;























































public class RefEnv implements TestEnvironment {
    TestFile    file;
    TestSuite   suite;
    RefDrv  driver;
    ObservedTask task;
    
    


    public RefEnv( TestFile f, TestSuite s, RefDrv d) {
        this.file = f;
        this.suite = s;
        this.driver = d;
    }

    


    public void runTest() {
        driver.p( file.name );
        try {
            file.startTime = driver.getCurrentTime();
            createContext();
            executeTestFile();
            file.endTime = driver.getCurrentTime();

            if (task.getExitValue() != 0) {
                if ( file.name.endsWith( "-n.js" )) {
                    file.passed = true;
                } else {                    
                    suite.passed   = false;
                    file.passed    = false;
                }                
            }
            if ( ! parseResult() ) {
                if ( file.name.endsWith( "-n.js" ) ) {
                    file.passed = true; 
                } else {                    
                    suite.passed   = false;
                    file.passed    = false;
                }                    
                file.exception = new String(task.getError());                
            }

        } catch ( Exception e ) {
            suite.passed = false;
            file.passed  = false;
            file.exception = "Unknown process exception.";




                              
        }
    }

    




    public Object createContext() {
        if ( driver.CODE_COVERAGE ) {
            String command = "coverage " +
                "/SaveMergeData /SaveMergeTextData "+
                driver.EXECUTABLE + " -f " + 
                driver.HELPER_FUNCTIONS.getAbsolutePath() + " -f " + 
                file.filePath;
            
            System.out.println( "command is " + command );
            
            task = new ObservedTask( command, this );
        } else {
            task = new ObservedTask( driver.EXECUTABLE + " -f " + 
                driver.HELPER_FUNCTIONS.getAbsolutePath() + " -f " + 
                file.filePath, this);
        }                
        return (Object) task;                
    }
    



    public Object executeTestFile() {
        try {
            task.exec();
        }   catch ( IOException e ) {
            driver.p( e.toString() );
            file.exception = e.toString();
            e.printStackTrace();
        }            
        return null;
    }

    



    public boolean parseResult() {
        String line;
        int i,j;

        BufferedReader br = new BufferedReader(new StringReader(
            new String(task.getInput())));
        try {
            do {
                line = br.readLine();
                driver.p( line );
                
                if (line == null) {
                    driver.p("\tERROR: No lines to read");
                    return false;
                }
            } while (!line.equals(sizeTag));
    
            if ((line = br.readLine()) == null) 
                return false;
            
            file.totalCases = Integer.valueOf(line).intValue();

            if ((line = br.readLine()) == null) {
                driver.p("\tERROR: No lines after " + sizeTag);
                return false;
            }
            
        } catch (NumberFormatException nfe) {
            driver.p("\tERROR: No integer after " + sizeTag);
            return false;
        } catch ( IOException e ) {
            driver.p( "Exception reading process output:" + e.toString() );
            file.exception  = e.toString();
            return false;
        }            
        
        for ( i = 0; i < file.totalCases; i++) {
            String values[] = new String[tags.length];
            try {
                for ( j = 0; j < tags.length; j++) {
                    values[j] = null;
                
                    if (!line.startsWith(tags[j])) {
                        driver.p("line didn't start with " + tags[j] +":"+line);
                        return false;
                    }                    
                    while (((line = br.readLine()) != null) && 
                        (!(line.startsWith(startTag))))
                    {
                        values[j] = (values[j] == null) ? line : (values[j] + 
                            "\n" + line);
                    }
                    if (values[j] == null) values[j] = "";
                }
                if ((line == null) && (i < file.totalCases - 1)) {
                    driver.p("line == null and " + i + "<" +
                        (file.totalCases - 1));
                    return false;
                }                
            } catch ( IOException e ) {
                driver.p( "Exception reading process output: " + e );
                file.exception = e.toString();
                return false;
            }                
                
            TestCase rt = new TestCase(values[0],values[1],values[4],values[2],
                values[3],values[5]);
                
            file.bugnumber = values[6];
            file.caseVector.addElement( rt );

            if ( rt.passed.equals("false") ) {
                if ( file.name.endsWith( "-n.js" ) ) {
                    this.file.passed = true;
                } else {                    
                    this.file.passed  = false;
                    this.suite.passed = false;
                }                    
            }

        }
        
        if ( file.totalCases == 0 ) {
            if ( file.name.endsWith( "-n.js" ) ) {
                this.file.passed = true;
            } else {                    
                this.file.reason  = "File contains no testcases. " + this.file.reason;
                this.file.passed  = false;
                this.suite.passed = false;
            }                    
        }
        
        return true;
    }
    



    public void close(){
        return;
    }

    


    public static final String tags[];
    


    public static final String sizeTag  = "<#TEST CASES SIZE>";
    


    public static final String startTag = "<#TEST CASE";
    


    public static final String endTag   = ">";

    


    static {
        String fields[] = { "PASSED", "NAME", "EXPECTED", "ACTUAL", "DESCRIPTION", "REASON",
                            "BUGNUMBER" };

        tags = new String[fields.length];

        for (int i = 0; i < fields.length; ++i) {
            tags[i] = startTag + " " + fields[i] + endTag;
        }
    }    
}