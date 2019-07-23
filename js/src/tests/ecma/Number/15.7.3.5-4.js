





































gTestfile = '15.7.3.5-4.js';













var SECTION = "15.7.3.5-4";
var VERSION = "ECMA_1";
startTest();
var TITLE   = "Number.NEGATIVE_INFINITY";

writeHeaderToLog( SECTION + " "+TITLE);

new TestCase( SECTION,
	      "var string = ''; for ( prop in Number ) { string += ( prop == 'NEGATIVE_INFINITY' ) ? prop : '' } string;",
	      "",
	      eval("var string = ''; for ( prop in Number ) { string += ( prop == 'NEGATIVE_INFINITY' ) ? prop : '' } string;")
  );

test();
