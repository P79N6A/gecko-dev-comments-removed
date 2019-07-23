package	com.netscape.javascript.qa.liveconnect.call;

import com.netscape.javascript.qa.liveconnect.*;
import netscape.javascript.*;












public class Call_001 extends LiveConnectTest {
	public Call_001() {
		super();
	}

	public static void main( String[] args ) {
		Call_001 test	= new Call_001();
		test.start();
	}

	public void	setupTestEnvironment() {
		super.setupTestEnvironment();
	}

	public void	executeTest() {
		Object data[] =	getDataArray();
		for	( int i	= 0; i < data.length; i++ )	{
			JSObject jsObject =	getJSObject( (Object[]) data[i] );
			call( jsObject, (Object[]) data[i] );
		}
	}

	






	public JSObject	getJSObject( Object[] data ) {
	    String constructor = data[0] +" = " + data[1];
	    JSObject theThis = (JSObject) global.eval( constructor );
		return theThis;
	}

    








	public void	call( JSObject	theThis, Object[] data ) {
		String exception = null;
		String method	 = (String)	data[2];
		Object args[]    = (Object[]) data[3];

		Object eValue	 = data[4];
		Object aValue	 = null;
		Class  eClass	 = null;
		Class  aClass	 = null;

		try	{
			aValue = theThis.call( method, (Object[]) args );
	    	if ( aValue	!= null	) {
				eClass = eValue.getClass();
				aClass = aValue.getClass();
			}
		} catch	( Exception	e )	{
			exception =	theThis	+".call( "	+ method + ", " + args+" ) threw "+
				e.toString();
			file.exception += exception;
			e.printStackTrace();
		}  finally {
			if ( aValue	== null	) {
			} else {
				
				addTestCase(					
					"[ getMember returned "	+ aValue +"	] "+
					theThis+".call( "+method+", "+args+" ).equals( "+eValue+" )",
					"true",
					aValue.equals(eValue) +"",
					exception );

				
				addTestCase	(
					"[ "+ aValue+".getClass() returned "+aClass.getName()+"] "+
					aClass.getName() +".equals(	" +eClass.getName()	+" )",
					"true",
					aClass.getName().equals(eClass.getName()) +"",
					exception );
			}
		}
	}

	















	public Object[]	getDataArray() {
		Object d0[]	= {
		    new String( "boo" ),            
			new	String(	"new Boolean()"),	
			new	String(	"valueOf" ),		
			null,	                        
			new	Boolean( "false" ),		
		};

		Object d1[]	= {
		    new String( "date" ),     
			new	String(	"new Date(0)"),
			new	String(	"getUTCFullYear" ),
			null,	                     
			new	Double( "1970" ),		
		};

	    Object dataArray[] = { d0, d1 };
		return dataArray;
	}
 }
