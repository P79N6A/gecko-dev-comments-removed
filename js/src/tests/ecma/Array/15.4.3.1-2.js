





































gTestfile = '15.4.3.1-2.js';











var SECTION = "15.4.3.1-1";
var VERSION = "ECMA_1";
startTest();
var TITLE   = "Array.prototype";

writeHeaderToLog( SECTION + " "+ TITLE);


var ARRAY_PROTO = Array.prototype;

new TestCase( SECTION, 
	      "var props = ''; for ( p in Array  ) { props += p } props",
	      "",
	      eval("var props = ''; for ( p in Array  ) { props += p } props") );

new TestCase( SECTION, 
	      "Array.prototype = null; Array.prototype",  
	      ARRAY_PROTO,
	      eval("Array.prototype = null; Array.prototype") );

new TestCase( SECTION, 
	      "delete Array.prototype",                  
	      false,      
	      delete Array.prototype );

new TestCase( SECTION, 
	      "delete Array.prototype; Array.prototype", 
	      ARRAY_PROTO,
	      eval("delete Array.prototype; Array.prototype") );

test();
