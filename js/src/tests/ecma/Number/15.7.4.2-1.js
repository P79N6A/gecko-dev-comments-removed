





































gTestfile = '15.7.4.2-1.js';



















var SECTION = "15.7.4.2-1";
var VERSION = "ECMA_1";
startTest();

writeHeaderToLog( SECTION + " Number.prototype.toString()");


new TestCase(SECTION,
	     "Number.prototype.toString()",      
	     "0",       
	     eval("Number.prototype.toString()") );

new TestCase(SECTION,
	     "typeof(Number.prototype.toString())",
	     "string",     
	     eval("typeof(Number.prototype.toString())") );

new TestCase(SECTION, 
	     "s = Number.prototype.toString; o = new Number(); o.toString = s; o.toString()", 
	     "0",         
	     eval("s = Number.prototype.toString; o = new Number(); o.toString = s; o.toString()") );

new TestCase(SECTION, 
	     "s = Number.prototype.toString; o = new Number(1); o.toString = s; o.toString()",
	     "1",         
	     eval("s = Number.prototype.toString; o = new Number(1); o.toString = s; o.toString()") );

new TestCase(SECTION, 
	     "s = Number.prototype.toString; o = new Number(-1); o.toString = s; o.toString()",
	     "-1",        
	     eval("s = Number.prototype.toString; o = new Number(-1); o.toString = s; o.toString()") );

new TestCase(SECTION,
	     "var MYNUM = new Number(255); MYNUM.toString(10)",         
	     "255",     
	     eval("var MYNUM = new Number(255); MYNUM.toString(10)") );

new TestCase(SECTION,
	     "var MYNUM = new Number(Number.NaN); MYNUM.toString(10)",  
	     "NaN",     
	     eval("var MYNUM = new Number(Number.NaN); MYNUM.toString(10)") );

new TestCase(SECTION,
	     "var MYNUM = new Number(Infinity); MYNUM.toString(10)",  
	     "Infinity",  
	     eval("var MYNUM = new Number(Infinity); MYNUM.toString(10)") );

new TestCase(SECTION,
	     "var MYNUM = new Number(-Infinity); MYNUM.toString(10)",  
	     "-Infinity",
	     eval("var MYNUM = new Number(-Infinity); MYNUM.toString(10)") );

test();
