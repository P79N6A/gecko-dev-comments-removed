




































gTestfile = '15.9.4.2-1.js';

















var SECTION = "15.9.4.2-1";       
var VERSION = "ECMA"; 
var TITLE   = "Regression Test for Date.parse";       
var BUGNUMBER = "http://bugzilla.mozilla.org/show_bug.cgi?id=4088";     

startTest();               

AddTestCase( "new Date('1/1/1999 12:30 AM').toString()",
	     new Date(1999,0,1,0,30).toString(),
	     new Date('1/1/1999 12:30 AM').toString() );

AddTestCase( "new Date('1/1/1999 12:30 PM').toString()",
	     new Date( 1999,0,1,12,30 ).toString(),
	     new Date('1/1/1999 12:30 PM').toString() );

AddTestCase( "new Date('1/1/1999 13:30 AM')",
	     "Invalid Date",
	     new Date('1/1/1999 13:30 AM').toString() );


AddTestCase( "new Date('1/1/1999 13:30 PM')",
	     "Invalid Date",
	     new Date('1/1/1999 13:30 PM').toString() );

test();       

