package com.netscape.javascript.qa.drivers;

import java.io.*;
import com.netscape.javascript.*;
import org.mozilla.javascript.*;
import org.mozilla.javascript.tools.shell.*;



















public class RhinoEnv implements TestEnvironment {
    public Global global;
    Object cx;
    Object result; 
    TestDriver driver;
    TestSuite suite;
    TestFile file;

    






    public RhinoEnv( TestFile f, TestSuite s, TestDriver d) {
        this.file = f;
        this.suite = s;
        this.driver = d;
    }
    








    public synchronized void runTest() {
        this.driver.p( file.name );
        try {
            cx = createContext();
            ((Context) cx).setOptimizationLevel( driver.OPT_LEVEL );
            ((Context) cx).setDebugLevel( driver.DEBUG_LEVEL );
            Object loadFn = executeTestFile(driver.HELPER_FUNCTIONS.getAbsolutePath());

            file.startTime = driver.getCurrentTime();
            result = executeTestFile( file.filePath );
            file.endTime = driver.getCurrentTime();
            parseResult();

        } catch ( Exception e ) {
            suite.passed = false;
            file.passed = false;
            file.exception +=  "file failed with exception:  " + e ;
        }
    }
    





    public Object createContext() {
        
        cx = new Context();
        ((Context) cx).enter();

        global = new Global();
        ((Context) cx).initStandardObjects(global);

        String[] names = { "print", "quit", "version", "load", "help",
                           "loadClass" };
        try {
            global.defineFunctionProperties(names, Main.class,
            ScriptableObject.DONTENUM);
        } catch (PropertyException e) {

            throw new Error(e.getMessage());
        }

        return cx;
    }
    public Object executeTestFile() {
        return null;
    }        
    
    











    public Object executeTestFile( String s ) {
        
        FileReader in = null;
        try {
            in = new FileReader( s );
        } catch (FileNotFoundException ex) {
            driver.p("couldn't open file " + s);
        }

        Object result = null;

        try {
            
            
            
            

            result = ((Scriptable) (((Context) cx).evaluateReader(
                (Scriptable) global, (Reader) in, s, 1, null)));

        }   catch (WrappedException we) {
            driver.p("Wrapped Exception:  "+
            we.getWrappedException().toString());
            result = we.getWrappedException().toString();
        }   catch (Exception jse) {
            driver.p("JavaScriptException: " + jse.getMessage());
            result = jse.getMessage();
        }

        return ( result );
    }
    















    public boolean parseResult() {
        FlattenedObject fo = null;
        
        if ( result instanceof Scriptable ) {
            fo = new FlattenedObject( (Scriptable) result );            
            
            try {
                file.totalCases = ((Number) fo.getProperty("length")).intValue();
                for ( int i = 0; i < file.totalCases; i++ ) {
                    Scriptable tc  = (Scriptable) ((Scriptable) result).get( i,
                    (Scriptable) result);

                    TestCase rt = new TestCase(
                        getString(tc.get( "passed", tc )),
                        getString(tc.get( "name", tc )),
                        getString(tc.get( "description", tc )),
                        getString(tc.get( "expect", tc )),
                        getString(tc.get( "actual", tc )),
                        getString(tc.get( "reason", tc ))
                    );

                    file.bugnumber= 
                        (getString(tc.get("bugnumber", tc))).startsWith
                            ("com.netscape.javascript") 
                            ? file.bugnumber
                            : getString(tc.get("bugnumber", tc));
                    
                    file.caseVector.addElement( rt );
                    if ( rt.passed.equals("false") ) {
                        this.file.passed = false;
                        this.suite.passed = false;
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
                
            } catch ( Exception e ) {                
                this.file.exception = "Got a Scriptable result, but failed "+
                    "parsing its arguments.  Exception: " + e.toString() +
                    " Flattened Object is: " + fo.toString();
                    
                this.file.passed = false;
                this.suite.passed = false;

                return false;
            }
        } else {
            
            
            this.file.exception = result.toString();

            

            if ( file.name.endsWith( "-n.js" ) ) {
                this.file.passed = true;
            } else {
                this.file.passed = false;
                this.suite.passed = false;
                return false;
            }
        }
       
        return true;
    }
    


    public void close() {
        try {
            ((Context) cx).exit();
         } catch ( Exception e ) {
            suite.passed = false;
            file.passed = false;
            file.exception =  "file failed with exception:  " + e ;
         }

    }
    





    public String getString( Object object ) {
         return (((Context) cx).toString( object ));
    }
}
