





































gTestfile = 'package-006.js';











var SECTION = "LiveConnect Packages";
var VERSION = "1_3";
var TITLE   = "LiveConnect Packages";

startTest();
writeHeaderToLog( SECTION + " "+ TITLE);

var util = java.util;
var v = new util.Vector();

new TestCase( SECTION,
	      "java.util[1]",
	      void 0,
	      java.util[1] );

test();

