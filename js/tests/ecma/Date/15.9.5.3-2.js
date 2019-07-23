




















































var SECTION = "15.9.5.3-2";
var VERSION = "ECMA_1";
startTest();
var TITLE   = "Date.prototype.valueOf";

writeHeaderToLog( SECTION + " "+ TITLE);

var TZ_ADJUST = TZ_DIFF * msPerHour;
var now = (new Date()).valueOf();
var UTC_29_FEB_2000 = TIME_2000 + 31*msPerDay + 28*msPerDay;
var UTC_1_JAN_2005 = TIME_2000 + TimeInYear(2000) + TimeInYear(2001)+
TimeInYear(2002)+TimeInYear(2003)+TimeInYear(2004);

addTestCase( now );
addTestCase( TIME_1970 );
addTestCase( TIME_1900 );
addTestCase( TIME_2000 );
addTestCase( UTC_29_FEB_2000 );
addTestCase( UTC_1_JAN_2005 );

test();

function addTestCase( t ) {
  new TestCase( SECTION,
		"(new Date("+t+").valueOf()",
		t,
		(new Date(t)).valueOf() );

  new TestCase( SECTION,
		"(new Date("+(t+1)+").valueOf()",
		t+1,
		(new Date(t+1)).valueOf() );

  new TestCase( SECTION,
		"(new Date("+(t-1)+").valueOf()",
		t-1,
		(new Date(t-1)).valueOf() );

  new TestCase( SECTION,
		"(new Date("+(t-TZ_ADJUST)+").valueOf()",
		t-TZ_ADJUST,
		(new Date(t-TZ_ADJUST)).valueOf() );

  new TestCase( SECTION,
		"(new Date("+(t+TZ_ADJUST)+").valueOf()",
		t+TZ_ADJUST,
		(new Date(t+TZ_ADJUST)).valueOf() );
}

function MyObject( value ) {
  this.value = value;
  this.valueOf = Date.prototype.valueOf;
  this.toString = new Function( "return this+\"\";");
  return this;
}
