





































gTestfile = '15.5.4.3-2.js';

















var SECTION = "15.5.4.3-2";
var VERSION = "ECMA_1";
startTest();
var TITLE   = "String.prototype.valueOf";

writeHeaderToLog( SECTION + " "+ TITLE);

new TestCase( SECTION,
	      "var valof=String.prototype.valueOf; astring=new String(); astring.valueOf = valof; astring.valof()",
	      "",
	      eval("var valof=String.prototype.valueOf; astring=new String(); astring.valueOf = valof; astring.valueOf()") );

new TestCase( SECTION,
	      "var valof=String.prototype.valueOf; astring=new String(0); astring.valueOf = valof; astring.valof()",
	      "0",
	      eval("var valof=String.prototype.valueOf; astring=new String(0); astring.valueOf = valof; astring.valueOf()") );

new TestCase( SECTION,
	      "var valof=String.prototype.valueOf; astring=new String('hello'); astring.valueOf = valof; astring.valof()",
	      "hello",
	      eval("var valof=String.prototype.valueOf; astring=new String('hello'); astring.valueOf = valof; astring.valueOf()") );

new TestCase( SECTION,
	      "var valof=String.prototype.valueOf; astring=new String(''); astring.valueOf = valof; astring.valof()",
	      "",
	      eval("var valof=String.prototype.valueOf; astring=new String(''); astring.valueOf = valof; astring.valueOf()") );







test();
