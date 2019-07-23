





































gTestfile = '15.4.2.2-2.js';































var SECTION = "15.4.2.2-2";
var VERSION = "ECMA_1";
startTest();
var TITLE   = "The Array Constructor:  new Array( len )";

writeHeaderToLog( SECTION + " "+ TITLE);

new TestCase( SECTION,
	      "(new Array(new Number(1073741823))).length",  
	      1,     
	      (new Array(new Number(1073741823))).length );

new TestCase( SECTION,
	      "(new Array(new Number(0))).length",           
	      1,     
	      (new Array(new Number(0))).length );

new TestCase( SECTION,
	      "(new Array(new Number(1000))).length",        
	      1,     
	      (new Array(new Number(1000))).length );

new TestCase( SECTION,
	      "(new Array('mozilla, larryzilla, curlyzilla')).length",
	      1, 
	      (new Array('mozilla, larryzilla, curlyzilla')).length );

new TestCase( SECTION,
	      "(new Array(true)).length",                    
	      1,     
	      (new Array(true)).length );

new TestCase( SECTION,
	      "(new Array(false)).length",                   
	      1,     
	      (new Array(false)).length);

new TestCase( SECTION,
	      "(new Array(new Boolean(true)).length",        
	      1,     
	      (new Array(new Boolean(true))).length );

new TestCase( SECTION,
	      "(new Array(new Boolean(false)).length",       
	      1,     
	      (new Array(new Boolean(false))).length );

test();
