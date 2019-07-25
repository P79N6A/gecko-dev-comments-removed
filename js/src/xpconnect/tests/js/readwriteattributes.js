











































 StartTest( "Read-Write Attributes" );

 



 var CONTRACTID = "@mozilla.org/js/xpc/test/ObjectReadWrite;1";
 var INAME   = Components.interfaces["nsIXPCTestObjectReadWrite"];

 var O = Components.classes[CONTRACTID].createInstance();
 o = O.QueryInterface( INAME );


 AddTestCase( "typeof Components.classes[" + CONTRACTID+"].createInstance()",
			   "object",
			   typeof O );

 AddTestCase( "typeof O.QueryInterface[" +INAME+"]",
			"object",
			typeof o );

 AddTestCase( "o.booleanProperty", true, o.booleanProperty );
 AddTestCase( "o.shortProperty", 32767, o.shortProperty );
 AddTestCase( "o.longProperty", 2147483647, o.longProperty );
 AddTestCase( "o.charProperty", "X", o.charProperty );
			  
 

 o.booleanProperty = false;
 o.shortProperty = -12345;
 o.longProperty = 1234567890;
 o.charProperty = "Z";

 AddTestCase( "o.booleanProperty", false, o.booleanProperty );
 AddTestCase( "o.shortProperty", -12345, o.shortProperty );
 AddTestCase( "o.longProperty", 1234567890, o.longProperty );
 AddTestCase( "o.charProperty", "Z", o.charProperty );

 
 
 SetAndTestBooleanProperty( false, false );
 SetAndTestBooleanProperty( 1, true );
 SetAndTestBooleanProperty( null, false );
 SetAndTestBooleanProperty( "A", true );
 SetAndTestBooleanProperty( undefined, false );
 SetAndTestBooleanProperty( [], true );
 SetAndTestBooleanProperty( {}, true );

 StopTest();

 function SetAndTestBooleanProperty( newValue, expectedValue ) {
	 o.booleanProperty = newValue;

	 AddTestCase( "o.booleanProperty = " + newValue +"; o.booleanProperty", 
				expectedValue, 
				o.booleanProperty );
 }
