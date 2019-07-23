











































 StartTest( "ReadOnly File Attributes" );

 



 var CONTRACTID = "@mozilla.org/js/xpc/test/ObjectReadOnly;1";
 var INAME   = Components.interfaces["nsIXPCTestObjectReadOnly"];

 var o = Components.classes[CONTRACTID].createInstance();
 o = o.QueryInterface( INAME );

 AddTestCase( "o.boolReadOnly", true, o.boolReadOnly );
 AddTestCase( "o.shortReadOnly", 32767, o.shortReadOnly );
 AddTestCase( "o.longReadOnly", 2147483647, o.longReadOnly );
 AddTestCase( "o.charReadOnly", "X", o.charReadOnly );
			  
 

 o.boolReadOnly = false;
 o.shortReadOnly = -12345;
 o.longReadOnly = 12345;
 o.charReadOnly = "Z";

 AddTestCase( "o.boolReadOnly", true, o.boolReadOnly );
 AddTestCase( "o.shortReadOnly", 32767, o.shortReadOnly );
 AddTestCase( "o.longReadOnly", 2147483647, o.longReadOnly );
 AddTestCase( "o.charReadOnly", "X", o.charReadOnly );

 StopTest();
