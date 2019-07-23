





































gTestfile = '15.5.4.2-3.js';



















var SECTION = "15.5.4.2-3";
var VERSION = "ECMA_1";
startTest();
var TITLE   = "String.prototype.toString";

writeHeaderToLog( SECTION + " "+ TITLE);

new TestCase( SECTION,
	      "var tostr=String.prototype.toString; astring=new String(); astring.toString = tostr; astring.toString()",
	      "",
	      eval("var tostr=String.prototype.toString; astring=new String(); astring.toString = tostr; astring.toString()") );
new TestCase( SECTION,
	      "var tostr=String.prototype.toString; astring=new String(0); astring.toString = tostr; astring.toString()",
	      "0",
	      eval("var tostr=String.prototype.toString; astring=new String(0); astring.toString = tostr; astring.toString()") );
new TestCase( SECTION,
	      "var tostr=String.prototype.toString; astring=new String('hello'); astring.toString = tostr; astring.toString()",
	      "hello",
	      eval("var tostr=String.prototype.toString; astring=new String('hello'); astring.toString = tostr; astring.toString()") );
new TestCase( SECTION,
	      "var tostr=String.prototype.toString; astring=new String(''); astring.toString = tostr; astring.toString()",
	      "",
	      eval("var tostr=String.prototype.toString; astring=new String(''); astring.toString = tostr; astring.toString()") );

test();
