





































gTestfile = '15.5.4.9-1.js';































var SECTION = "15.5.4.9-1";
var VERSION = "ECMA_1";
startTest();
var TITLE   = "String.prototype.substring( start )";

writeHeaderToLog( SECTION + " "+ TITLE);

new TestCase( SECTION,  "String.prototype.substring.length",        2,          String.prototype.substring.length );
new TestCase( SECTION,  "delete String.prototype.substring.length", false,      delete String.prototype.substring.length );
new TestCase( SECTION,  "delete String.prototype.substring.length; String.prototype.substring.length", 2,      eval("delete String.prototype.substring.length; String.prototype.substring.length") );





new TestCase(   SECTION,
		"var s = new String('this is a string object'); typeof s.substring()",
		"string",
		eval("var s = new String('this is a string object'); typeof s.substring()") );

new TestCase(   SECTION,
		"var s = new String(''); s.substring()",
		"",
		eval("var s = new String(''); s.substring()") );


new TestCase(   SECTION,
		"var s = new String('this is a string object'); s.substring()",
		"this is a string object",
		eval("var s = new String('this is a string object'); s.substring()") );

new TestCase(   SECTION,
		"var s = new String('this is a string object'); s.substring(NaN)",
		"this is a string object",
		eval("var s = new String('this is a string object'); s.substring(NaN)") );


new TestCase(   SECTION,
		"var s = new String('this is a string object'); s.substring(-0.01)",
		"this is a string object",
		eval("var s = new String('this is a string object'); s.substring(-0.01)") );


new TestCase(   SECTION,
		"var s = new String('this is a string object'); s.substring(s.length)",
		"",
		eval("var s = new String('this is a string object'); s.substring(s.length)") );

new TestCase(   SECTION,
		"var s = new String('this is a string object'); s.substring(s.length+1)",
		"",
		eval("var s = new String('this is a string object'); s.substring(s.length+1)") );


new TestCase(   SECTION,
		"var s = new String('this is a string object'); s.substring(Infinity)",
		"",
		eval("var s = new String('this is a string object'); s.substring(Infinity)") );

new TestCase(   SECTION,
		"var s = new String('this is a string object'); s.substring(-Infinity)",
		"this is a string object",
		eval("var s = new String('this is a string object'); s.substring(-Infinity)") );




new TestCase(   SECTION,
		"var s = new Array(1,2,3,4,5); s.substring = String.prototype.substring; s.substring()",
		"1,2,3,4,5",
		eval("var s = new Array(1,2,3,4,5); s.substring = String.prototype.substring; s.substring()") );

new TestCase(   SECTION,
		"var s = new Array(1,2,3,4,5); s.substring = String.prototype.substring; s.substring(true)",
		",2,3,4,5",
		eval("var s = new Array(1,2,3,4,5); s.substring = String.prototype.substring; s.substring(true)") );

new TestCase(   SECTION,
		"var s = new Array(1,2,3,4,5); s.substring = String.prototype.substring; s.substring('4')",
		"3,4,5",
		eval("var s = new Array(1,2,3,4,5); s.substring = String.prototype.substring; s.substring('4')") );

new TestCase(   SECTION,
		"var s = new Array(); s.substring = String.prototype.substring; s.substring('4')",
		"",
		eval("var s = new Array(); s.substring = String.prototype.substring; s.substring('4')") );


new TestCase(   SECTION,
		"var obj = new Object(); obj.substring = String.prototype.substring; obj.substring(8)",
		"Object]",
		eval("var obj = new Object(); obj.substring = String.prototype.substring; obj.substring(8)") );


new TestCase(   SECTION,
		"var obj = new Function(); obj.substring = String.prototype.substring; obj.toString = Object.prototype.toString; obj.substring(8)",
		"Function]",
		eval("var obj = new Function(); obj.substring = String.prototype.substring; obj.toString = Object.prototype.toString; obj.substring(8)") );

new TestCase(   SECTION,
		"var obj = new Number(NaN); obj.substring = String.prototype.substring; obj.substring(false)",
		"NaN",
		eval("var obj = new Number(NaN); obj.substring = String.prototype.substring; obj.substring(false)") );


new TestCase(   SECTION,
		"var obj = Math; obj.substring = String.prototype.substring; obj.substring(Math.PI)",
		"ject Math]",
		eval("var obj = Math; obj.substring = String.prototype.substring; obj.substring(Math.PI)") );



new TestCase(   SECTION,
		"var obj = new Boolean(); obj.substring = String.prototype.substring; obj.substring(new Array())",
		"false",
		eval("var obj = new Boolean(); obj.substring = String.prototype.substring; obj.substring(new Array())") );



new TestCase( SECTION,
	      "var obj = new MyObject( null ); obj.substring(0)",
	      "null",
	      eval( "var obj = new MyObject( null ); obj.substring(0)") );


test();

function MyObject( value ) {
  this.value = value;
  this.substring = String.prototype.substring;
  this.toString = new Function ( "return this.value+''" );
}
