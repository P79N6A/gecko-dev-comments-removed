package	com.netscape.javascript.qa.liveconnect.member;

import com.netscape.javascript.qa.liveconnect.*;
import netscape.javascript.*;











public class Member_002	extends	LiveConnectTest	{
	public Member_002()	{
		super();
	}

	public static void main( String[] args ) {
		Member_002 test	= new Member_002();
		test.start();
	}

	public void	setupTestEnvironment() {
		super.setupTestEnvironment();
	}

	public void	executeTest() {
		Object data[] =	getDataArray();
		for	( int i	= 0; i < data.length; i++ )	{
			JSObject jsObject =	getJSObject( (Object[]) data[i] );
			setMember( (Object[]) data[i] );
			getMember( jsObject, (Object[]) data[i] );
			removeMember( jsObject, (Object[]) data[i] );
		}
	}
	
	






	public JSObject	getJSObject( Object[] data ) {
	    String constructor = data[0] +" = " + data[1];
	    JSObject theThis = (JSObject) global.eval( constructor );
		return theThis;
	}
	
	








	public void setMember( Object[] data ) {
        Object result = null;	    
	    String evalString = (String) data[0] + "." + (String) data[2] +" = "+ 
	            (String) data[3];
	    try {
    	    result = global.eval( evalString );
        } catch ( Exception e ) {
            exception = evalString + " threw " + e.toString();
            file.exception += exception;
            e.printStackTrace();
        } finally {
            addTestCase(
                "[ " + evalString+ " returned "+result+" ] " +
                result +".equals( " + data[5] +")",
                "true",
                result.equals(data[5]) +"",
                exception );
        }            
    }	    

    






	public void	getMember( JSObject	theThis, Object[] data ) {
		String exception = null;
		String property	 = (String)	data[2];
		Object eValue	 = data[6];
		Object aValue	 = null;
		Class  eClass	 = null;
		Class  aClass	 = null;
		String eType     = (String) data[7];
		String aType     = null;

		try	{
			aValue = theThis.getMember(	property );
	    	if ( aValue	!= null	) {
				eClass = eValue.getClass();
				aClass = aValue.getClass();
				aType  = (String) global.eval( "typeof " + ((String) data[0]) +
				    "." + ((String) data[2]) );
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
					theThis+".getMember( " +property+" ).equals( "+eValue+"	)",
					"true",
					aValue.equals(eValue) +"",
					exception );

				
				addTestCase	(
					"[ "+ aValue+".getClass() returned "+aClass.getName()+"] "+
					aClass.getName() +".equals(	" +eClass.getName()	+" )",
					"true",
					aClass.getName().equals(eClass.getName()) +"",
					exception );

                
                addTestCase(
                    "[ typeof " +aValue +" returned " + aType +" ] "+
                    aType +" .equals( " +eType + " )",
                    "true",
                    aType.equals( eType ) +"",
                    exception );
			}
		}
	}	
	








	public void removeMember( JSObject theThis, Object[] data ) {
	    String exception = null;
	    String property = (String) data[2];
	    Object eValue   = data[8];
	    Object aValue   = null;
	    Class  eClass   = null;
	    Class  aClass   = null;
	    String eType    = (String) data[9];
	    String aType    = null;
	    
		try	{
			theThis.removeMember( property );
			aValue = theThis.getMember( property );
	    	if ( aValue	!= null	) {
				eClass = eValue.getClass();
				aClass = aValue.getClass();
				aType  = (String) global.eval( "typeof " + ((String) data[0]) +
				    "." + ((String) data[2]) );
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
					"[ after removing, getMember returned "	+ aValue +"	] "+
					theThis+".getMember( " +property+" ).equals( "+eValue+"	)",
					"true",
					aValue.equals(eValue) +"",
					exception );

				
				addTestCase	(
					"[ after removing, "+ aValue+".getClass() returned "+aClass.getName()+"] "+
					aClass.getName() +".equals(	" +eClass.getName()	+" )",
					"true",
					aClass.getName().equals(eClass.getName()) +"",
					exception );

                
                addTestCase(
                    "[ after removing, typeof " +aValue +" returned " + aType +" ] "+
                    aType +" .equals( " +eType + " )",
                    "true",
                    aType.equals( eType ) +"",
                    exception );
			}
		}
	    
    }	    
	
	



















	public Object[]	getDataArray() {
		Object d0[]	= {
		    new String( "boo" ),            
			new	String(	"new Boolean()"),	
			new	String(	"foo" ),			
			new	String(	"\"bar\"" ),	    
			new	String(	"undefined"	),		
			new String( "bar" ),            
			new	String(	"bar" ),			
			new	String(	"string"),			
			new String( "undefined"),       
			new String( "undefined")        
		};
		
		Object d1[] = {
		    new String( "num" ),
		    new String( "new Number(12345)" ),
		    new String( "someProperty" ),
		    new String( ".02134" ),
		    new String( "undefined" ),
		    new Double ( "0.02134"),
		    new Double ( "0.02134"),
		    new String ("number"),
			new String( "undefined"),       
			new String( "undefined")        
		    
        };		    
        
        Object d2[] = {            
            new String( "number" ),
            new String( "Number" ),
            new String( "POSITIVE_INFINITY" ),
            new String( "0" ),
            new Double( Double.POSITIVE_INFINITY ),
            new Double("0" ),
            new Double( Double.POSITIVE_INFINITY ),
            new String( "number" ) ,
            new Double( Double.POSITIVE_INFINITY ),
            new String( "number" ) 
        };            
        
	    Object dataArray[] = { d0, d1, d2 };
		return dataArray;
	}
 }
