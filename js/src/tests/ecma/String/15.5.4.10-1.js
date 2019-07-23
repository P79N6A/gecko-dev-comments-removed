





































gTestfile = '15.5.4.10-1.js';









































var SECTION = "15.5.4.10-1";
var VERSION = "ECMA_1";
startTest();
var TITLE   = "String.prototype.substring( start, end )";

writeHeaderToLog( SECTION + " "+ TITLE);

new TestCase( SECTION,  "String.prototype.substring.length",        2,          String.prototype.substring.length );
new TestCase( SECTION,  "delete String.prototype.substring.length", false,      delete String.prototype.substring.length );
new TestCase( SECTION,  "delete String.prototype.substring.length; String.prototype.substring.length", 2,      eval("delete String.prototype.substring.length; String.prototype.substring.length") );





new TestCase(   SECTION,
		"var s = new String('this is a string object'); typeof s.substring()",
		"string",
		eval("var s = new String('this is a string object'); typeof s.substring()") );

new TestCase(   SECTION,
		"var s = new String(''); s.substring(1,0)",
		"",
		eval("var s = new String(''); s.substring(1,0)") );

new TestCase(   SECTION,
		"var s = new String('this is a string object'); s.substring(true, false)",
		"t",
		eval("var s = new String('this is a string object'); s.substring(false, true)") );

new TestCase(   SECTION,
		"var s = new String('this is a string object'); s.substring(NaN, Infinity)",
		"this is a string object",
		eval("var s = new String('this is a string object'); s.substring(NaN, Infinity)") );


new TestCase(   SECTION,
		"var s = new String('this is a string object'); s.substring(Infinity, NaN)",
		"this is a string object",
		eval("var s = new String('this is a string object'); s.substring(Infinity, NaN)") );


new TestCase(   SECTION,
		"var s = new String('this is a string object'); s.substring(Infinity, Infinity)",
		"",
		eval("var s = new String('this is a string object'); s.substring(Infinity, Infinity)") );

new TestCase(   SECTION,
		"var s = new String('this is a string object'); s.substring(-0.01, 0)",
		"",
		eval("var s = new String('this is a string object'); s.substring(-0.01,0)") );


new TestCase(   SECTION,
		"var s = new String('this is a string object'); s.substring(s.length, s.length)",
		"",
		eval("var s = new String('this is a string object'); s.substring(s.length, s.length)") );

new TestCase(   SECTION,
		"var s = new String('this is a string object'); s.substring(s.length+1, 0)",
		"this is a string object",
		eval("var s = new String('this is a string object'); s.substring(s.length+1, 0)") );


new TestCase(   SECTION,
		"var s = new String('this is a string object'); s.substring(-Infinity, -Infinity)",
		"",
		eval("var s = new String('this is a string object'); s.substring(-Infinity, -Infinity)") );




new TestCase(   SECTION,
		"var s = new Array(1,2,3,4,5); s.substring = String.prototype.substring; s.substring(Infinity,-Infinity)",
		"1,2,3,4,5",
		eval("var s = new Array(1,2,3,4,5); s.substring = String.prototype.substring; s.substring(Infinity,-Infinity)") );

new TestCase(   SECTION,
		"var s = new Array(1,2,3,4,5); s.substring = String.prototype.substring; s.substring(true, false)",
		"1",
		eval("var s = new Array(1,2,3,4,5); s.substring = String.prototype.substring; s.substring(true, false)") );

new TestCase(   SECTION,
		"var s = new Array(1,2,3,4,5); s.substring = String.prototype.substring; s.substring('4', '5')",
		"3",
		eval("var s = new Array(1,2,3,4,5); s.substring = String.prototype.substring; s.substring('4', '5')") );



new TestCase(   SECTION,
		"var obj = new Object(); obj.substring = String.prototype.substring; obj.substring(8,0)",
		"[object ",
		eval("var obj = new Object(); obj.substring = String.prototype.substring; obj.substring(8,0)") );

new TestCase(   SECTION,
		"var obj = new Object(); obj.substring = String.prototype.substring; obj.substring(8,obj.toString().length)",
		"Object]",
		eval("var obj = new Object(); obj.substring = String.prototype.substring; obj.substring(8, obj.toString().length)") );


new TestCase(   SECTION,
		"var obj = new Function(); obj.substring = String.prototype.substring; obj.toString = Object.prototype.toString; obj.substring(8, Infinity)",
		"Function]",
		eval("var obj = new Function(); obj.substring = String.prototype.substring; obj.toString = Object.prototype.toString; obj.substring(8,Infinity)") );

new TestCase(   SECTION,
		"var obj = new Number(NaN); obj.substring = String.prototype.substring; obj.substring(Infinity, NaN)",
		"NaN",
		eval("var obj = new Number(NaN); obj.substring = String.prototype.substring; obj.substring(Infinity, NaN)") );


new TestCase(   SECTION,
		"var obj = Math; obj.substring = String.prototype.substring; obj.substring(Math.PI, -10)",
		"[ob",
		eval("var obj = Math; obj.substring = String.prototype.substring; obj.substring(Math.PI, -10)") );



new TestCase(   SECTION,
		"var obj = new Boolean(); obj.substring = String.prototype.substring; obj.substring(new Array(), new Boolean(1))",
		"f",
		eval("var obj = new Boolean(); obj.substring = String.prototype.substring; obj.substring(new Array(), new Boolean(1))") );



new TestCase( SECTION,
	      "var obj = new MyObject( void 0 ); obj.substring(0, 100)",
	      "undefined",
	      eval( "var obj = new MyObject( void 0 ); obj.substring(0,100)") );

test();

function MyObject( value ) {
  this.value = value;
  this.substring = String.prototype.substring;
  this.toString = new Function ( "return this.value+''" );
}
