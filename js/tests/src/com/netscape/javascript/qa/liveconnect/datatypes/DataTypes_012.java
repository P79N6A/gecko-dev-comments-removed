
package com.netscape.javascript.qa.liveconnect.datatypes;

import com.netscape.javascript.qa.liveconnect.*;

























public class DataTypes_012 extends LiveConnectTest {
    public DataTypes_012() {
        super();
    }

    public static void main( String[] args ) {
        DataTypes_012 test = new DataTypes_012();
        test.start();
    }

    public void setupTestEnvironment() {
        super.setupTestEnvironment();
        global.eval( "var DT = "+
            "Packages.com.netscape.javascript.qa.liveconnect.DataTypeClass");
		global.eval( "var dt = new DT();" );			
	}

	public void	executeTest() {
		doArrayTest( "DT.staticGetLongArray();", true );
		doArrayTest( "DT.PUB_STATIC_ARRAY_LONG;", true);
		doArrayTest( "dt.getLongArray();", true );
		doArrayTest( "dt.PUB_ARRAY_LONG;", false );
	}
	









	
	public void	doArrayTest( String	command, boolean shouldEqual ) {
		long	array[]	 = DataTypeClass.PUB_STATIC_ARRAY_LONG;
		long	jsArray[];
		int		jsArray_length;
		
		try	{
			
			global.eval( "var jsArray =	" +	command	);
			
            
            jsArray = (long[]) global.getMember( "jsArray" );

            
            jsArray_length =
                ((Double) global.eval("jsArray.length")).intValue();

            
            

            for ( int i = 0; i < jsArray_length; i++ ) {
                
                

                Double item = (Double) global.eval( "jsArray[" + i +"];" );

                addTestCase(
					"[jsArray = " + command +"] "+                
                    "global.eval(\"jsArray["+i+"]\").equals( array["+i+"])",
                    "true",
                    (item.equals(new Double(array[i]))) +"",
                    "" );
            }

        } catch ( Exception e ) {
            e.printStackTrace();
            file.exception = e.toString();
            jsArray_length = 0;
            jsArray = null;
        }

        

        addTestCase(
            "[jsArray = " + command +"] "+        
            "jsArray = global.getMember( \"jsArray\"); "+
            "jsArray == array",
			( shouldEqual ) ? "true" : "false",
            (jsArray == array ) +"",
            "" );
    }
 }