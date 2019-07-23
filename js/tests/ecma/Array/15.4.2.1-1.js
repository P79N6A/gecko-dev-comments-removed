





































gTestfile = '15.4.2.1-1.js';






























var SECTION = "15.4.2.1-1";
var VERSION = "ECMA_1";
startTest();
var TITLE   = "The Array Constructor:  new Array( item0, item1, ...)";

writeHeaderToLog( SECTION + " "+ TITLE);

new TestCase( SECTION,
	      "typeof new Array(1,2)",       
	      "object",          
	      typeof new Array(1,2) );

new TestCase( SECTION,
	      "(new Array(1,2)).toString",   
	      Array.prototype.toString,   
	      (new Array(1,2)).toString );

new TestCase( SECTION,
	      "var arr = new Array(1,2,3); arr.getClass = Object.prototype.toString; arr.getClass()",
	      "[object Array]",
	      eval("var arr = new Array(1,2,3); arr.getClass = Object.prototype.toString; arr.getClass()") );

new TestCase( SECTION,
	      "(new Array(1,2)).length",     
	      2,                 
	      (new Array(1,2)).length );

new TestCase( SECTION,
	      "var arr = (new Array(1,2)); arr[0]", 
	      1,          
	      eval("var arr = (new Array(1,2)); arr[0]") );

new TestCase( SECTION,
	      "var arr = (new Array(1,2)); arr[1]", 
	      2,          
	      eval("var arr = (new Array(1,2)); arr[1]") );

new TestCase( SECTION,
	      "var arr = (new Array(1,2)); String(arr)", 
	      "1,2", 
	      eval("var arr = (new Array(1,2)); String(arr)") );

test();
