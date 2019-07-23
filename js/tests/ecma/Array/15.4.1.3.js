





































gTestfile = '15.4.1.3.js';

















var SECTION = "15.4.1.3";
var VERSION = "ECMA_1";
startTest();
var TITLE   = "Array Constructor Called as a Function:  Array()";

writeHeaderToLog( SECTION + " "+ TITLE);

new TestCase(   SECTION,
		"typeof Array()",
		"object",
		typeof Array() );

new TestCase(   SECTION,
		"MYARR = new Array();MYARR.getClass = Object.prototype.toString;MYARR.getClass()",
		"[object Array]",
		eval("MYARR = Array();MYARR.getClass = Object.prototype.toString;MYARR.getClass()") );

new TestCase(   SECTION,
		"(Array()).length",
		0,         
		(Array()).length );

new TestCase(   SECTION,
		"Array().toString()",
		"",
		Array().toString() );

test();
