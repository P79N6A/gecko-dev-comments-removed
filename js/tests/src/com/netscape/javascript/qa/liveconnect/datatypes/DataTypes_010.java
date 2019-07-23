
package com.netscape.javascript.qa.liveconnect.datatypes;

import com.netscape.javascript.qa.liveconnect.*;

























public class DataTypes_010 extends LiveConnectTest {
    public DataTypes_010() {
        super();
    }

    public static void main( String[] args ) {
        DataTypes_010 test = new DataTypes_010();
        test.start();
    }

    public void setupTestEnvironment() {
        super.setupTestEnvironment();
        global.eval( "var DT = "+
            "Packages.com.netscape.javascript.qa.liveconnect.DataTypeClass");
		global.eval( "var dt = new DT();" );            
    }

	public void	executeTest() {
		doArrayTest( "DT.staticGetDoubleArray();", true );
		doArrayTest( "DT.PUB_STATIC_ARRAY_DOUBLE;", true);
		doArrayTest( "dt.getDoubleArray();", true );
		doArrayTest( "dt.PUB_ARRAY_DOUBLE;", false );
	}
	
	









	
	public void	doArrayTest( String	command, boolean shouldEqual ) {
		double	array[]	 = DataTypeClass.PUB_STATIC_ARRAY_DOUBLE;
        double  jsArray[];
        int     jsArray_length;

        try {
            
            global.eval( "var jsArray = " + command );

            
            jsArray = (double[]) global.getMember( "jsArray" );

            
            jsArray_length =
                ((Double) global.eval("jsArray.length")).intValue();

            
            

            for ( int i = 0; i < jsArray_length; i++ ) {
                
                

                Double item = (Double) global.eval( "jsArray[" + i +"];" );

                addTestCase(
					"[ jsArray = "	+ command +"] "+
					"global.eval( \"var	jsArray	= "	+ command +")"+
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
			"[jsArray = "+ command +"] "+		
			"jsArray = global.getMember( \"jsArray\"); "+
			"jsArray == array",
			( shouldEqual ) ? "true" : "false",
			(jsArray == array )	+"",
			"" );
	}
 }