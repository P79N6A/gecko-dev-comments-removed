





































gTestfile = '10.2.1.js';

















var SECTION = "10.2.1";
var VERSION = "ECMA_1";
startTest();
var TITLE   = "Global Code";

writeHeaderToLog( SECTION + " "+ TITLE);

var THIS = this;

new TestCase( SECTION,
	      "this +''",
	      GLOBAL,
	      THIS + "" );

var GLOBAL_PROPERTIES = new Array();
var i = 0;

for ( p in this ) {
  GLOBAL_PROPERTIES[i++] = p;
}

for ( i = 0; i < GLOBAL_PROPERTIES.length; i++ ) {
  new TestCase( SECTION,
		GLOBAL_PROPERTIES[i] +" == void 0",
		false,
		eval("GLOBAL_PROPERTIES["+i+"] == void 0"));
}

test();
