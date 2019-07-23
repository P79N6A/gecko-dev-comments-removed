





































gTestfile = '15.3.2.1-1.js';


















var SECTION = "15.3.2.1";
var VERSION = "ECMA_1";
startTest();
var TITLE   = "The Function Constructor";

writeHeaderToLog( SECTION + " "+ TITLE);

var MyObject = new Function( "value", "this.value = value; this.valueOf = new Function( 'return this.value' ); this.toString = new Function( 'return String(this.value);' )" );

new TestCase( SECTION,
	      "MyObject.__proto__ == Function.prototype",   
	      true,
	      MyObject.__proto__ == Function.prototype );

test();
