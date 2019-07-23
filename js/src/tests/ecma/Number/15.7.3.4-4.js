





































gTestfile = '15.7.3.4-4.js';













var SECTION = "15.7.3.4-4";
var VERSION = "ECMA_1";
startTest();
var TITLE   = "Number.NaN";

writeHeaderToLog( SECTION + " " + TITLE);

new TestCase( SECTION,
	      "var string = ''; for ( prop in Number ) { string += ( prop == 'NaN' ) ? prop : '' } string;",
	      "",
	      eval("var string = ''; for ( prop in Number ) { string += ( prop == 'NaN' ) ? prop : '' } string;")
  );

test();
