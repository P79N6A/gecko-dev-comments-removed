





































gTestfile = '15.3.1.1-2.js';


















var SECTION = "15.3.1.1-2";
var VERSION = "ECMA_1";
startTest();
var TITLE   = "The Function Constructor Called as a Function";

writeHeaderToLog( SECTION + " "+ TITLE);

var myfunc2 =  Function("a, b, c",   "return a+b+c" );
var myfunc3 =  Function("a,b", "c",  "return a+b+c" );

myfunc2.toString = Object.prototype.toString;
myfunc3.toString = Object.prototype.toString;


new TestCase( SECTION, 
	      "myfunc2.__proto__",                        
	      Function.prototype,    
	      myfunc2.__proto__ );

new TestCase( SECTION, 
	      "myfunc3.__proto__",                        
	      Function.prototype,    
	      myfunc3.__proto__ );

test();
