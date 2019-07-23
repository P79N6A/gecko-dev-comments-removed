





































gTestfile = '15.7.3.1-3.js';













var VERSION = "ECMA_1";
startTest();
var SECTION = "15.7.3.1-3";
var TITLE   = "Number.prototype";

writeHeaderToLog( SECTION + " Number.prototype:  DontEnum Attribute");

new TestCase(
  SECTION,
  "var string = ''; for ( prop in Number ) { string += ( prop == 'prototype' ) ? prop: '' } string;",
  "",
  eval("var string = ''; for ( prop in Number ) { string += ( prop == 'prototype' ) ? prop : '' } string;")
  );

test();
