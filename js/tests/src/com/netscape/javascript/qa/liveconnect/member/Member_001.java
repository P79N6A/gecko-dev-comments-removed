package	com.netscape.javascript.qa.liveconnect.member;

import com.netscape.javascript.qa.liveconnect.*;
import netscape.javascript.*;













public class Member_001	extends	LiveConnectTest	{
	public Member_001()	{
		super();
	}

	public static void main( String[] args ) {
		Member_001 test	= new Member_001();
		test.start();
	}

	public void	setupTestEnvironment() {
		super.setupTestEnvironment();
	}

	public void	executeTest() {
		Object data[] =	getDataArray();
		for	( int i	= 0; i < data.length; i++ )	{
			JSObject jsObject =	getJSObject( (Object[]) data[i] );

			
			getMember( jsObject, (Object[])	data[i], ((Object[]) data[i])[4] );

			
			setMember( jsObject, (Object[])	data[i]	);

			
			getMember( jsObject, (Object[])	data[i], ((Object[]) data[i])[5] );
		}
	}
	public JSObject	getJSObject( Object[] data ) {
		return (JSObject) global.eval( data[0] +" = " + data[1] );
	}		 
	

















	public Object[]	getDataArray() {
		Object d0[]	= {
		    new String( "d0" ),            
			new	String(	"new Boolean()"),	
			new	String(	"foo" ),			
			new	String(	"bar" ),			
			new	String(	"undefined"	),		
			new	String(	"bar" ),			
			"java.lang.String",				
			new	String(	"object")			
		};

		Object d1[]	= {
		    new String( "d1" ),            
			new	String(	"new String(\"JavaScript\")"),	
			new	String(	"foo" ),			
			new	Boolean( "true" ),			
			new	String(	"undefined"	),		
			new	Boolean( "true" ),			
			"java.lang.Boolean",				
			new	String(	"object")			
		};
		
		Object d2[] = {
		    new String( "d2" ),            
			new	String(	"new Number(12345)"), 
			new	String(	"foo" ),			
			new	Double( "0.2134" ),			
			new	String(	"undefined"	),		
			new	Double( "0.2134" ),			
			"java.lang.Double",				
			new	String(	"object")			
		};

		Object d3[] = {
		    new String( "d3" ),            
			new	String(	"new Number(12345)"), 
			new	String(	"foo" ),			
			new	Integer( "987654" ),			
			new	String(	"undefined"	),		
			new	Integer( "987654" ),			
			"java.lang.Integer",				
			new	String(	"object")			
		};
    
        Object d4[] = {
            new String( "d4" ),
            new String ( "new Object()" ),
            new String( "property" ),
            global,
            new String( "undefined" ),
            global,
            "netscape.javascript.JSObject",
            new String ( "object" )
        };            

		Object dataArray[] = { d0, d1, d3, d4 };
		return dataArray;
	}

    




	public void	getMember( JSObject	theThis, Object[] data,	Object value ) {
		String exception = "";
		String property	 = (String)	data[2];
		Object eValue	 = value;
		Object aValue	 = null;
		Class  eClass	 = null;
		Class  aClass	 = null;
		
		try	{
			aValue = theThis.getMember(	property );
		   
			if ( aValue	!= null	) {
				eClass = eValue.getClass();
				aClass = aValue.getClass();
			}				 
			
		} catch	( Exception	e )	{
			exception =	theThis	+".getMember( "	+ property + " ) threw " +
				e.toString();
			file.exception += exception;
			e.printStackTrace();
		}  finally {
			if ( aValue	== null	) {
			} else {
				
				addTestCase( 
					"[ getMember returned "	+ aValue +"	] "+
					data[0]+".getMember( " +property+" ).equals( "+eValue+"	)",
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

    



	public void	setMember( JSObject	theThis, Object[] data ) {
		String exception = "";
		String result	 = "passed";
		String property	 = (String)	data[2];
		Object value	 = data[3];
		
		try	{
			theThis.setMember( property, value );
		}  catch ( Exception e ) {
			result = "failed!";
			exception =	"("+ theThis+").setMember( " + property	+","+ value	+" )	"+
				"threw " + e.toString();
			file.exception += exception;
			e.printStackTrace();
		} finally {
			addTestCase(
				"("+theThis+").setMember( "+property +", "+	value +" )",
				"passed",
				result,
				exception );
		}
	}
 }
