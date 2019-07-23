package com.netscape.javascript.qa.liveconnect.datatypes;

import com.netscape.javascript.qa.liveconnect.*;

























public class DataTypes_014 extends LiveConnectTest {
    public DataTypes_014() {
        super();
    }

    public static void main( String[] args ) {
        DataTypes_014 test = new DataTypes_014();
        test.start();
    }

    public void setupTestEnvironment() {
        super.setupTestEnvironment();
        global.eval( "var DT = "+
            "Packages.com.netscape.javascript.qa.liveconnect.DataTypeClass");
		global.eval( "var dt = new DT();" );			
	}

	public void	executeTest() {
		doArrayTest( "DT.staticGetFloatArray();", true );
		doArrayTest( "DT.PUB_STATIC_ARRAY_FLOAT;", true);
		doArrayTest( "dt.getFloatArray();", true );
		doArrayTest( "dt.PUB_ARRAY_FLOAT;", false );
	}
	









	
	public void	doArrayTest( String	command, boolean shouldEqual	) {
		float	array[]	 = DataTypeClass.PUB_STATIC_ARRAY_FLOAT;
		float	jsArray[];
		int		jsArray_length;
		
		try	{
			
			global.eval( "var jsArray =	" +	command	);

            
            jsArray = (float[]) global.getMember( "jsArray" );

            
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
			"[jsArray = "+ command +"] "+		
			"jsArray = global.getMember( \"jsArray\"); "+
			"jsArray == array",
			( shouldEqual ) ? "true" : "false",
			(jsArray == array )	+"",
			"" );
	}
 }