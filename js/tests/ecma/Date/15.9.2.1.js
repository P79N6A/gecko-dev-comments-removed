

















































var VERSION =   "ECMA_1";
startTest();
var SECTION =   "15.9.2.1";
var TITLE =     "Date Constructor used as a function";
var TYPEOF  =   "string";
var TOLERANCE = 1000;

writeHeaderToLog("15.9.2.1 The Date Constructor Called as a Function:  " +
		 "Date( year, month, date, hours, minutes, seconds, ms )" );

var TODAY = new Date();



new TestCase( SECTION, "Date(1970,0,1,0,0,0,0)",            (new Date()).toString(),    Date(1970,0,1,0,0,0,0) );
new TestCase( SECTION, "Date(1969,11,31,15,59,59,999)",     (new Date()).toString(),    Date(1969,11,31,15,59,59,999));
new TestCase( SECTION, "Date(1969,11,31,16,0,0,0)",         (new Date()).toString(),    Date(1969,11,31,16,0,0,0));
new TestCase( SECTION, "Date(1969,11,31,16,0,0,1)",         (new Date()).toString(),    Date(1969,11,31,16,0,0,1));

  
  new TestCase( SECTION, "Date(1999,11,15,59,59,999)",        (new Date()).toString(),    Date(1999,11,15,59,59,999));
new TestCase( SECTION, "Date(1999,11,16,0,0,0,0)",          (new Date()).toString(),    Date(1999,11,16,0,0,0,0));
new TestCase( SECTION, "Date(1999,11,31,23,59,59,999)",     (new Date()).toString(),    Date(1999,11,31,23,59,59,999) );
new TestCase( SECTION, "Date(2000,0,1,0,0,0,0)",            (new Date()).toString(),    Date(2000,0,0,0,0,0,0) );
new TestCase( SECTION, "Date(2000,0,1,0,0,0,1)",            (new Date()).toString(),    Date(2000,0,0,0,0,0,1) );



new TestCase( SECTION, "Date(1899,11,31,23,59,59,999)",     (new Date()).toString(),    Date(1899,11,31,23,59,59,999));
new TestCase( SECTION, "Date(1900,0,1,0,0,0,0)",            (new Date()).toString(),    Date(1900,0,1,0,0,0,0) );
new TestCase( SECTION, "Date(1900,0,1,0,0,0,1)",            (new Date()).toString(),    Date(1900,0,1,0,0,0,1) );
new TestCase( SECTION, "Date(1899,11,31,16,0,0,0,0)",       (new Date()).toString(),    Date(1899,11,31,16,0,0,0,0));



new TestCase( SECTION, "Date( 2000,1,29,0,0,0,0)",         (new Date()).toString(),    Date(2000,1,29,0,0,0,0));
new TestCase( SECTION, "Date( 2000,1,28,23,59,59,999)",    (new Date()).toString(),    Date( 2000,1,28,23,59,59,999));
new TestCase( SECTION, "Date( 2000,1,27,16,0,0,0)",        (new Date()).toString(),    Date(2000,1,27,16,0,0,0));


new TestCase( SECTION, "Date(2004,11,31,23,59,59,999)",     (new Date()).toString(),    Date(2004,11,31,23,59,59,999));
new TestCase( SECTION, "Date(2005,0,1,0,0,0,0)",            (new Date()).toString(),    Date(2005,0,1,0,0,0,0) );
new TestCase( SECTION, "Date(2005,0,1,0,0,0,1)",            (new Date()).toString(),    Date(2005,0,1,0,0,0,1) );
new TestCase( SECTION, "Date(2004,11,31,16,0,0,0,0)",       (new Date()).toString(),    Date(2004,11,31,16,0,0,0,0));


new TestCase( SECTION, "Date(2031,11,31,23,59,59,999)",     (new Date()).toString(),    Date(2031,11,31,23,59,59,999));
new TestCase( SECTION, "Date(2032,0,1,0,0,0,0)",            (new Date()).toString(),    Date(2032,0,1,0,0,0,0) );
new TestCase( SECTION, "Date(2032,0,1,0,0,0,1)",            (new Date()).toString(),    Date(2032,0,1,0,0,0,1) );
new TestCase( SECTION, "Date(2031,11,31,16,0,0,0,0)",       (new Date()).toString(),    Date(2031,11,31,16,0,0,0,0));

test();
