





































gTestfile = '15.7.3.6-4.js';












var SECTION = "15.7.3.6-4";
var VERSION = "ECMA_1";
startTest();
var TITLE   = "Number.POSITIVE_INFINITY";

writeHeaderToLog( SECTION + " "+TITLE);

new TestCase( SECTION,
	      "var string = ''; for ( prop in Number ) { string += ( prop == 'POSITIVE_INFINITY' ) ? prop : '' } string;",
	      "",
	      eval("var string = ''; for ( prop in Number ) { string += ( prop == 'POSITIVE_INFINITY' ) ? prop : '' } string;")
  );


test();
