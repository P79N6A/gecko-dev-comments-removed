





































gTestfile = '9.9-1.js';



















var VERSION = "ECMA_1";
startTest();
var SECTION = "9.9-1";

writeHeaderToLog( SECTION + " Type Conversion: ToObject" );

new TestCase( SECTION, "(Object(true)).__proto__",  Boolean.prototype,      (Object(true)).__proto__ );

new TestCase( SECTION, "(Object(true)).__proto__",  Boolean.prototype,      (Object(true)).__proto__ );

new TestCase( SECTION, "(Object(0)).__proto__",     Number.prototype,      (Object(0)).__proto__ );

new TestCase( SECTION, "(Object(-0)).__proto__",    Number.prototype,      (Object(-0)).__proto__ );

new TestCase( SECTION, "(Object(1)).__proto__",     Number.prototype,      (Object(1)).__proto__ );

new TestCase( SECTION, "(Object(-1)).__proto__",    Number.prototype,      (Object(-1)).__proto__ );

new TestCase( SECTION, "(Object(Number.MAX_VALUE)).__proto__",  Number.prototype,               (Object(Number.MAX_VALUE)).__proto__ );

new TestCase( SECTION, "(Object(Number.MIN_VALUE)).__proto__",  Number.prototype, (Object(Number.MIN_VALUE)).__proto__ );

new TestCase( SECTION, "(Object(Number.POSITIVE_INFINITY)).__proto__",  Number.prototype,               (Object(Number.POSITIVE_INFINITY)).__proto__ );

new TestCase( SECTION, "(Object(Number.NEGATIVE_INFINITY)).__proto__",  Number.prototype,   (Object(Number.NEGATIVE_INFINITY)).__proto__ );

new TestCase( SECTION, "(Object(Number.NaN)).__proto__",    Number.prototype,          (Object(Number.NaN)).__proto__ );

new TestCase( SECTION, "(Object('a string')).__proto__",    String.prototype,   (Object("a string")).__proto__ );

new TestCase( SECTION, "(Object('')).__proto__",            String.prototype,   (Object("")).__proto__ );

new TestCase( SECTION, "(Object('\\r\\t\\b\\n\\v\\f')).__proto__", String.prototype,   (Object("\\r\\t\\b\\n\\v\\f")).__proto__ );

new TestCase( SECTION,  "Object( '\\\'\\\"\\' ).__proto__",      String.prototype,   (Object("\'\"\\")).__proto__ );

new TestCase( SECTION, "(Object( new MyObject(true) )).toString()",  "[object Object]",       eval("(Object( new MyObject(true) )).toString()") );

test();

function MyObject( value ) {
  this.value = value;
  this.valueOf = new Function ( "return this.value" );
}
