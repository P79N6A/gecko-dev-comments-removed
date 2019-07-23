





































gTestfile = '15.7.3.3-4.js';














var SECTION = "15.7.3.3-4";
var VERSION = "ECMA_1";
startTest();

writeHeaderToLog( SECTION + " Number.MIN_VALUE:  DontEnum Attribute");

new TestCase( SECTION,
	      "var string = ''; for ( prop in Number ) { string += ( prop == 'MIN_VALUE' ) ? prop : '' } string;",
	      "",
	      eval("var string = ''; for ( prop in Number ) { string += ( prop == 'MIN_VALUE' ) ? prop : '' } string;")
  );

test();
