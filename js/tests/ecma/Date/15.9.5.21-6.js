





































gTestfile = '15.9.5.21-6.js';














var SECTION = "15.9.5.21";
var VERSION = "ECMA_1";
startTest();
var TITLE   = "Date.prototype.getUTCMilliseconds()";

writeHeaderToLog( SECTION + " "+ TITLE);

addTestCase( UTC_FEB_29_2000 );

test();

function addTestCase( t ) {
  new TestCase( SECTION,
		"(new Date("+t+")).getUTCMilliseconds()",
		msFromTime(t),
		(new Date(t)).getUTCMilliseconds() );
}
