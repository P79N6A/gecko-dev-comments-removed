





































gTestfile = '15.7.4.3-1.js';














var SECTION = "15.7.4.3-1";
var VERSION = "ECMA_1";
startTest();


writeHeaderToLog( SECTION + " Number.prototype.valueOf()");


new TestCase("SECTION",
	     "Number.prototype.valueOf()",       
	     0,       
	     eval("Number.prototype.valueOf()") );

new TestCase("SECTION",
	     "(new Number(1)).valueOf()",        
	     1,      
	     eval("(new Number(1)).valueOf()") );

new TestCase("SECTION",
	     "(new Number(-1)).valueOf()",       
	     -1,     
	     eval("(new Number(-1)).valueOf()") );

new TestCase("SECTION",
	     "(new Number(0)).valueOf()",        
	     0,      
	     eval("(new Number(0)).valueOf()") );

new TestCase("SECTION",
	     "(new Number(Number.POSITIVE_INFINITY)).valueOf()",
	     Number.POSITIVE_INFINITY,
	     eval("(new Number(Number.POSITIVE_INFINITY)).valueOf()") );

new TestCase("SECTION",
	     "(new Number(Number.NaN)).valueOf()", 
	     Number.NaN,
	     eval("(new Number(Number.NaN)).valueOf()") );

new TestCase("SECTION",
	     "(new Number()).valueOf()",        
	     0,      
	     eval("(new Number()).valueOf()") );

test();
