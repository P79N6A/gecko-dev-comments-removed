





































gTestfile = '15.4.2.3.js';














var SECTION = "15.4.2.3";
var VERSION = "ECMA_1";
startTest();
var TITLE   = "The Array Constructor:  new Array()";

writeHeaderToLog( SECTION + " "+ TITLE);

new TestCase( SECTION,
	      "new   Array() +''",       
	      "",                
	      (new Array()) +"" );

new TestCase( SECTION,
	      "typeof new Array()",      
	      "object",          
	      (typeof new Array()) );

new TestCase( SECTION,
	      "var arr = new Array(); arr.getClass = Object.prototype.toString; arr.getClass()",
	      "[object Array]",
	      eval("var arr = new Array(); arr.getClass = Object.prototype.toString; arr.getClass()") );

new TestCase( SECTION,
	      "(new Array()).length",    
	      0,                 
	      (new Array()).length );

new TestCase( SECTION,
	      "(new Array()).toString == Array.prototype.toString",  
	      true,      
	      (new Array()).toString == Array.prototype.toString );

new TestCase( SECTION,
	      "(new Array()).join  == Array.prototype.join",         
	      true,      
	      (new Array()).join  == Array.prototype.join );

new TestCase( SECTION,
	      "(new Array()).reverse == Array.prototype.reverse",    
	      true,      
	      (new Array()).reverse  == Array.prototype.reverse );

new TestCase( SECTION,
	      "(new Array()).sort  == Array.prototype.sort",        
	      true,      
	      (new Array()).sort  == Array.prototype.sort );

test();
