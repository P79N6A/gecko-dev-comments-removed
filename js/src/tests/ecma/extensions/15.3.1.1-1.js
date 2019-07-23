





































gTestfile = '15.3.1.1-1.js';

















var SECTION = "15.3.1.1-1";
var VERSION = "ECMA_1";
startTest();
var TITLE   = "The Function Constructor Called as a Function";

writeHeaderToLog( SECTION + " "+ TITLE);

var MyObject = Function( "value", "this.value = value; this.valueOf =  Function( 'return this.value' ); this.toString =  Function( 'return String(this.value);' )" );


var myfunc = Function();
myfunc.toString = Object.prototype.toString;




myfunc.toString = Object.prototype.toString;

new TestCase( SECTION, 
	      "MyObject.__proto__ == Function.prototype",    
	      true,  
	      MyObject.__proto__ == Function.prototype );

test();


