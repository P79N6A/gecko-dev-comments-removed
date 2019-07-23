





































gTestfile = '15-2.js';


















var SECTION = "15-2";
var VERSION = "ECMA_1";
startTest();
var TITLE   = "Native ECMAScript Objects";

writeHeaderToLog( SECTION + " "+ TITLE);

new TestCase( SECTION,  "Object.__proto__",   Function.prototype,   Object.__proto__ );
new TestCase( SECTION,  "Array.__proto__",    Function.prototype,   Array.__proto__ );
new TestCase( SECTION,  "String.__proto__",   Function.prototype,   String.__proto__ );
new TestCase( SECTION,  "Boolean.__proto__",  Function.prototype,   Boolean.__proto__ );
new TestCase( SECTION,  "Number.__proto__",   Function.prototype,   Number.__proto__ );
new TestCase( SECTION,  "Date.__proto__",     Function.prototype,   Date.__proto__ );
new TestCase( SECTION,  "TestCase.__proto__", Function.prototype,   TestCase.__proto__ );

new TestCase( SECTION,  "eval.__proto__",     Function.prototype,   eval.__proto__ );
new TestCase( SECTION,  "Math.pow.__proto__", Function.prototype,   Math.pow.__proto__ );
new TestCase( SECTION,  "String.prototype.indexOf.__proto__", Function.prototype,   String.prototype.indexOf.__proto__ );

test();
