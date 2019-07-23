





































gTestfile = '15.5.4.2.js';










var SECTION = "15.5.4.2";
var VERSION = "ECMA_1";
startTest();
var TITLE   = "String.prototype.tostring";

writeHeaderToLog( SECTION + " "+ TITLE);

new TestCase(   SECTION,
		"String.prototype.toString() == String.prototype.valueOf()",
		true,
		String.prototype.toString() == String.prototype.valueOf() );

new TestCase(   SECTION, "String.prototype.toString()",     "",     String.prototype.toString() );
new TestCase(   SECTION, "String.prototype.toString.length",    0,  String.prototype.toString.length );


new TestCase(   SECTION,
		"TESTSTRING = new String();TESTSTRING.valueOf() == TESTSTRING.toString()",
		true,
		eval("TESTSTRING = new String();TESTSTRING.valueOf() == TESTSTRING.toString()") );
new TestCase(   SECTION,
		"TESTSTRING = new String(true);TESTSTRING.valueOf() == TESTSTRING.toString()",
		true,
		eval("TESTSTRING = new String(true);TESTSTRING.valueOf() == TESTSTRING.toString()") );
new TestCase(   SECTION,
		"TESTSTRING = new String(false);TESTSTRING.valueOf() == TESTSTRING.toString()",
		true,
		eval("TESTSTRING = new String(false);TESTSTRING.valueOf() == TESTSTRING.toString()") );
new TestCase(   SECTION,
		"TESTSTRING = new String(Math.PI);TESTSTRING.valueOf() == TESTSTRING.toString()",
		true,
		eval("TESTSTRING = new String(Math.PI);TESTSTRING.valueOf() == TESTSTRING.toString()") );
new TestCase(   SECTION,
		"TESTSTRING = new String();TESTSTRING.valueOf() == TESTSTRING.toString()",
		true,
		eval("TESTSTRING = new String();TESTSTRING.valueOf() == TESTSTRING.toString()") );

test();
