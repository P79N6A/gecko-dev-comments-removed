package com.netscape.javascript.qa.liveconnect.datatypes;

import com.netscape.javascript.qa.liveconnect.*;

























public class DataTypes_015 extends LiveConnectTest {
    public DataTypes_015() {
        super();
    }

    public static void main( String[] args ) {
        DataTypes_015 test = new DataTypes_015();
        test.start();
    }

    public void setupTestEnvironment() {
        super.setupTestEnvironment();
        global.eval( "var DT = "+
            "Packages.com.netscape.javascript.qa.liveconnect.DataTypeClass;");
		global.eval( "var dt = new DT();" );			
	}

	public void	executeTest() {
		doArrayTest( "DT.staticGetObjectArray();", true );
		doArrayTest( "DT.PUB_STATIC_ARRAY_OBJECT;", true);
		doArrayTest( "dt.getObjectArray();", true );
		doArrayTest( "dt.PUB_ARRAY_OBJECT;", false );
	}
	









	
	public void	doArrayTest( String	command, boolean shouldEqual	) {
		Object	array[]	 = DataTypeClass.PUB_STATIC_ARRAY_OBJECT;
		Object	jsArray[];
		int		jsArray_length;
		
		try	{
			
			global.eval( "var jsArray =	" +	command	);

            
            jsArray = (Object[]) global.getMember( "jsArray" );

            
            jsArray_length =
                ((Double) global.eval("jsArray.length")).intValue();

            
            

            for ( int i = 0; i < jsArray_length; i++ ) {
                
                
            
                Object item = (Object) global.eval( "jsArray[" + i +"];" );

                addTestCase(
					"[jsArray = " + command +"] "+
                    "global.eval(\"jsArray["+i+"]\").equals( array["+i+"])",
			        shouldEqual+"",
                    item.equals(array[i]) +"",
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
			( shouldEqual )+"",
			(jsArray == array )	+"",
			exception );
	}
 }