





































gTestfile = '15.9.5.3-2.js';
















var SECTION = "15.9.5.3-2";
var VERSION = "ECMA_1";
startTest();
var TITLE   = "Date.prototype.valueOf";

writeHeaderToLog( SECTION + " "+ TITLE);

addTestCase( TIME_NOW );
addTestCase( TIME_1970 );
addTestCase( TIME_1900 );
addTestCase( TIME_2000 );
addTestCase( UTC_FEB_29_2000 );
addTestCase( UTC_JAN_1_2005 );

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
