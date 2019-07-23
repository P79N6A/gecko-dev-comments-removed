





































gTestfile = '15.5.4.1.js';










var SECTION = "15.5.4.1";
var VERSION = "ECMA_1";
startTest();
var TITLE   = "String.prototype.constructor";

writeHeaderToLog( SECTION + " "+ TITLE);

new TestCase( SECTION, "String.prototype.constructor == String",  true, String.prototype.constructor == String );

new TestCase( SECTION, "var STRING = new String.prototype.constructor('hi'); STRING.getClass = Object.prototype.toString; STRING.getClass()",
	      "[object String]",
	      eval("var STRING = new String.prototype.constructor('hi'); STRING.getClass = Object.prototype.toString; STRING.getClass()") );

test();
