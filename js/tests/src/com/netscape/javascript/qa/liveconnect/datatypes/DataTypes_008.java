
package	com.netscape.javascript.qa.liveconnect.datatypes;

import com.netscape.javascript.qa.liveconnect.*;























public class DataTypes_008 extends LiveConnectTest {
	public DataTypes_008() {
		super();
	}

	public static void main( String[] args ) {
		DataTypes_008 test = new DataTypes_008();
		test.start();
	}

	public void	setupTestEnvironment() {
		super.setupTestEnvironment();
		global.eval( "var DT = "+
			"Packages.com.netscape.javascript.qa.liveconnect.DataTypeClass");
		global.eval( "var dt = new DT();" );			
	}

	public void	executeTest() {
		doArrayTest( "DT.PUB_STATIC_ARRAY_BYTE;", true);
		doArrayTest( "dt.getByteArray();", true );
		doArrayTest( "dt.PUB_ARRAY_BYTE;", false );
	}
	









	public void	doArrayTest( String command, boolean shouldEqual) {
		byte	array[]	 = DataTypeClass.PUB_STATIC_ARRAY_BYTE;
		byte	jsArray[];
		int		jsArray_length;
		
		try	{
			
			global.eval( "var jsArray =	" +	command	);
			
			
			jsArray	= (byte[]) global.getMember( "jsArray" );
			
			
			jsArray_length = 
				((Double) global.eval("jsArray.length")).intValue();
				
			
			

			for	( int i	= 0; i < jsArray_length; i++ ) {
				
				
			
				Double item	= (Double) global.eval(	"jsArray[" + i +"];" );

				addTestCase(
					"[jsArray = " + command +"] "+
					"global.eval( \"var	jsArray	= "	+ command +")"+
					"global.eval(\"jsArray["+i+"]\").equals( array["+i+"])",
					"true",
					(item.equals(new Double(array[i])))	+"",
					"" );
			}					 
				
		} catch	( Exception	e )	{
			e.printStackTrace();
			file.exception = e.toString();
			jsArray_length = 0;
			jsArray	= null;
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