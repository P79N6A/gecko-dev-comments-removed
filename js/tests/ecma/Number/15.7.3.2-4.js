





































gTestfile = '15.7.3.2-4.js';












var SECTION = "15.7.3.2-4";
var VERSION = "ECMA_1";
startTest();
var TITLE   = "Number.MAX_VALUE:  DontEnum Attribute";
writeHeaderToLog( SECTION + " "+ TITLE);

new TestCase( SECTION,
	      "var string = ''; for ( prop in Number ) { string += ( prop == 'MAX_VALUE' ) ? prop : '' } string;",
	      "",
	      eval("var string = ''; for ( prop in Number ) { string += ( prop == 'MAX_VALUE' ) ? prop : '' } string;")
  );

test();
