





































gTestfile = '15.6.3.1-1.js';

















var SECTION = "15.6.3.1-1";
var VERSION = "ECMA_1";
startTest();
var TITLE   = "Boolean.prototype";

writeHeaderToLog( SECTION + " "+ TITLE);


var array = new Array();
var item = 0;

new TestCase( SECTION,
	      "var str='';for ( p in Boolean ) { str += p } str;",
	      "",
	      eval("var str='';for ( p in Boolean ) { str += p } str;") );
test();
