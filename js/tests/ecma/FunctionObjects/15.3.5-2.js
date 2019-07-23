





































gTestfile = '15.3.5-2.js';




































var SECTION = "15.3.5-2";
var VERSION = "ECMA_1";
startTest();
var TITLE   = "Properties of Function Instances";

writeHeaderToLog( SECTION + " "+TITLE);

var MyObject = new Function( 'a', 'b', 'c', 'this.a = a; this.b = b; this.c = c; this.value = a+b+c; this.valueOf = new Function( "return this.value" )' );

new TestCase( SECTION, "MyObject.length",                       3,          MyObject.length );
new TestCase( SECTION, "typeof MyObject.prototype",             "object",   typeof MyObject.prototype );
new TestCase( SECTION, "typeof MyObject.prototype.constructor", "function", typeof MyObject.prototype.constructor );
new TestCase( SECTION, "MyObject.arguments",                     null,       MyObject.arguments );

test();
