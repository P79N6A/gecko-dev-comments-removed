
package	com.netscape.javascript.qa.liveconnect.datatypes;

import com.netscape.javascript.qa.liveconnect.*;


























public class DataTypes_017 extends LiveConnectTest {
	public DataTypes_017() {
		super();
	}

	public static void main( String[] args ) {
		DataTypes_017 test = new DataTypes_017();
		test.start();
	}

	public void	setupTestEnvironment() {
		super.setupTestEnvironment();
		
		global.eval( "var DT = "+
			"Packages.com.netscape.javascript.qa.liveconnect.DataTypeClass");
		global.eval( "var dt = new DT();" );
	}

	
	

	String jsVals[]	= {	"0", "3.14159",	"-1.159", "-0.01", "0.1",
						"4294967295", "4294967296",	"-4294967296",
						"2147483647", "2147483648",	 "-2147483648" };

    




















     
    public void	executeTest() {
        for ( int i = 0; i < jsVals.length; i++ ) {
            doSetterTest(jsVals[i],
                         "dt.setDouble", "dt.getDouble", "dt.PUB_DOUBLE",
                         (new Double(jsVals[i]).doubleValue() > Double.MAX_VALUE 
                          ? (Object) EXCEPTION
                          : (new Double(jsVals[i]).doubleValue() < Double.MIN_VALUE
                             ? (Object) EXCEPTION : new Double( jsVals[i] ))
                          ));
						  









































						  
		}

	}
	












    public void	doSetterTest(String jsValue, String setter, String getter,
                             String field, Object eResult )
    {
        String setMethod = setter +"(" + jsValue +");";
        String getMethod = getter +	"();";
        String setterResult = "No exception thrown";
        Double getterResult = null;
        Double fieldResult = null;
        Object expectedResult = null;
        boolean eq = false;
        
        try {
            eq = eResult.getClass().equals(Class.forName("java.lang.String"));
        }
        catch (ClassNotFoundException e) {
            addTestCase (setMethod + " driver error.",
                         "very", "bad", file.exception);
        }
        
        if (eq) {
            try {
                global.eval( setMethod );
            } catch ( Exception e ) {
                setterResult = EXCEPTION;
                file.exception = e.toString();
                e.printStackTrace();
            } finally {
                addTestCase(setMethod +" should throw a JSException",
                            EXCEPTION,
                            setterResult,
                            file.exception );
            }
        } else {    
		
            try	{
                
                global.eval( setMethod );
                
                
                getterResult = (Double)	global.eval( getMethod );
                
                
                fieldResult	= (Double) global.eval(	field );
                
            } catch	( Exception	e )	{
                e.printStackTrace();
                file.exception = e.toString();
            } finally {
                addTestCase("[value: " + getterResult +"; expected:	" +
                            expectedResult +"] "+ setMethod + getMethod +
                            "( " + expectedResult + ").equals(" + 
                            getterResult +")", "true",
                            expectedResult.equals(getterResult)	+ "",
                            file.exception);

                addTestCase("[value: " + fieldResult + "; expected: " +
                            expectedResult + "] " + setMethod + field +
                            "; (" + expectedResult +").equals(" +
                            fieldResult +")", "true",
                            expectedResult.equals(fieldResult) +"",
                            file.exception );
            }
        }   
    }
	
    String EXCEPTION = "JSException expected";
}
