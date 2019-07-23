





































gTestfile = '15.5.4.8-1.js';




























var SECTION = "15.5.4.8-1";
var VERSION = "ECMA_1";
startTest();
var TITLE   = "String.prototype.split";

writeHeaderToLog( SECTION + " "+ TITLE);

new TestCase( SECTION,  "String.prototype.split.length",        2,          String.prototype.split.length );
new TestCase( SECTION,  "delete String.prototype.split.length", false,      delete String.prototype.split.length );
new TestCase( SECTION,  "delete String.prototype.split.length; String.prototype.split.length", 2,      eval("delete String.prototype.split.length; String.prototype.split.length") );





new TestCase(   SECTION,
		"var s = new String('this is a string object'); typeof s.split()",
		"object",
		eval("var s = new String('this is a string object'); typeof s.split()") );

new TestCase(   SECTION,
		"var s = new String('this is a string object'); Array.prototype.getClass = Object.prototype.toString; (s.split()).getClass()",
		"[object Array]",
		eval("var s = new String('this is a string object'); Array.prototype.getClass = Object.prototype.toString; (s.split()).getClass()") );

new TestCase(   SECTION,
		"var s = new String('this is a string object'); s.split().length",
		1,
		eval("var s = new String('this is a string object'); s.split().length") );

new TestCase(   SECTION,
		"var s = new String('this is a string object'); s.split()[0]",
		"this is a string object",
		eval("var s = new String('this is a string object'); s.split()[0]") );


new TestCase(   SECTION,
		"var obj = new Object(); obj.split = String.prototype.split; typeof obj.split()",
		"object",
		eval("var obj = new Object(); obj.split = String.prototype.split; typeof obj.split()") );

new TestCase(   SECTION,
		"var obj = new Object(); obj.split = String.prototype.split; Array.prototype.getClass = Object.prototype.toString; obj.getClass()",
		"[object Array]",
		eval("var obj = new Object(); obj.split = String.prototype.split; Array.prototype.getClass = Object.prototype.toString; obj.split().getClass()") );

new TestCase(   SECTION,
		"var obj = new Object(); obj.split = String.prototype.split; obj.split().length",
		1,
		eval("var obj = new Object(); obj.split = String.prototype.split; obj.split().length") );

new TestCase(   SECTION,
		"var obj = new Object(); obj.split = String.prototype.split; obj.split()[0]",
		"[object Object]",
		eval("var obj = new Object(); obj.split = String.prototype.split; obj.split()[0]") );


new TestCase(   SECTION,
		"var obj = new Function(); obj.split = String.prototype.split; typeof obj.split()",
		"object",
		eval("var obj = new Function(); obj.split = String.prototype.split; typeof obj.split()") );

new TestCase(   SECTION,
		"var obj = new Function(); obj.split = String.prototype.split; Array.prototype.getClass = Object.prototype.toString; obj.getClass()",
		"[object Array]",
		eval("var obj = new Function(); obj.split = String.prototype.split; Array.prototype.getClass = Object.prototype.toString; obj.split().getClass()") );

new TestCase(   SECTION,
		"var obj = new Function(); obj.split = String.prototype.split; obj.split().length",
		1,
		eval("var obj = new Function(); obj.split = String.prototype.split; obj.split().length") );

new TestCase(   SECTION,
		"var obj = new Function(); obj.split = String.prototype.split; obj.toString = Object.prototype.toString; obj.split()[0]",
		"[object Function]",
		eval("var obj = new Function(); obj.split = String.prototype.split; obj.toString = Object.prototype.toString; obj.split()[0]") );


new TestCase(   SECTION,
		"var obj = new Number(NaN); obj.split = String.prototype.split; typeof obj.split()",
		"object",
		eval("var obj = new Number(NaN); obj.split = String.prototype.split; typeof obj.split()") );

new TestCase(   SECTION,
		"var obj = new Number(Infinity); obj.split = String.prototype.split; Array.prototype.getClass = Object.prototype.toString; obj.getClass()",
		"[object Array]",
		eval("var obj = new Number(Infinity); obj.split = String.prototype.split; Array.prototype.getClass = Object.prototype.toString; obj.split().getClass()") );

new TestCase(   SECTION,
		"var obj = new Number(-1234567890); obj.split = String.prototype.split; obj.split().length",
		1,
		eval("var obj = new Number(-1234567890); obj.split = String.prototype.split; obj.split().length") );

new TestCase(   SECTION,
		"var obj = new Number(-1e21); obj.split = String.prototype.split; obj.split()[0]",
		"-1e+21",
		eval("var obj = new Number(-1e21); obj.split = String.prototype.split; obj.split()[0]") );



new TestCase(   SECTION,
		"var obj = Math; obj.split = String.prototype.split; typeof obj.split()",
		"object",
		eval("var obj = Math; obj.split = String.prototype.split; typeof obj.split()") );

new TestCase(   SECTION,
		"var obj = Math; obj.split = String.prototype.split; Array.prototype.getClass = Object.prototype.toString; obj.getClass()",
		"[object Array]",
		eval("var obj = Math; obj.split = String.prototype.split; Array.prototype.getClass = Object.prototype.toString; obj.split().getClass()") );

new TestCase(   SECTION,
		"var obj = Math; obj.split = String.prototype.split; obj.split().length",
		1,
		eval("var obj = Math; obj.split = String.prototype.split; obj.split().length") );

new TestCase(   SECTION,
		"var obj = Math; obj.split = String.prototype.split; obj.split()[0]",
		"[object Math]",
		eval("var obj = Math; obj.split = String.prototype.split; obj.split()[0]") );


new TestCase(   SECTION,
		"var obj = new Array(1,2,3,4,5); obj.split = String.prototype.split; typeof obj.split()",
		"object",
		eval("var obj = new Array(1,2,3,4,5); obj.split = String.prototype.split; typeof obj.split()") );

new TestCase(   SECTION,
		"var obj = new Array(1,2,3,4,5); obj.split = String.prototype.split; Array.prototype.getClass = Object.prototype.toString; obj.getClass()",
		"[object Array]",
		eval("var obj = new Array(1,2,3,4,5); obj.split = String.prototype.split; Array.prototype.getClass = Object.prototype.toString; obj.split().getClass()") );

new TestCase(   SECTION,
		"var obj = new Array(1,2,3,4,5); obj.split = String.prototype.split; obj.split().length",
		1,
		eval("var obj = new Array(1,2,3,4,5); obj.split = String.prototype.split; obj.split().length") );

new TestCase(   SECTION,
		"var obj = new Array(1,2,3,4,5); obj.split = String.prototype.split; obj.split()[0]",
		"1,2,3,4,5",
		eval("var obj = new Array(1,2,3,4,5); obj.split = String.prototype.split; obj.split()[0]") );



new TestCase(   SECTION,
		"var obj = new Boolean(); obj.split = String.prototype.split; typeof obj.split()",
		"object",
		eval("var obj = new Boolean(); obj.split = String.prototype.split; typeof obj.split()") );

new TestCase(   SECTION,
		"var obj = new Boolean(); obj.split = String.prototype.split; Array.prototype.getClass = Object.prototype.toString; obj.getClass()",
		"[object Array]",
		eval("var obj = new Boolean(); obj.split = String.prototype.split; Array.prototype.getClass = Object.prototype.toString; obj.split().getClass()") );

new TestCase(   SECTION,
		"var obj = new Boolean(); obj.split = String.prototype.split; obj.split().length",
		1,
		eval("var obj = new Boolean(); obj.split = String.prototype.split; obj.split().length") );

new TestCase(   SECTION,
		"var obj = new Boolean(); obj.split = String.prototype.split; obj.split()[0]",
		"false",
		eval("var obj = new Boolean(); obj.split = String.prototype.split; obj.split()[0]") );


test();
