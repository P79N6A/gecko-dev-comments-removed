





































gTestfile = '15.3.2.1-2.js';











var SECTION = "15.3.2.1-2";
var VERSION = "ECMA_1";
startTest();
var TITLE   = "The Function Constructor";

writeHeaderToLog( SECTION + " "+ TITLE);


var myfunc1 = new Function("a","b","c", "return a+b+c" );
var myfunc2 = new Function("a, b, c",   "return a+b+c" );
var myfunc3 = new Function("a,b", "c",  "return a+b+c" );

myfunc1.toString = Object.prototype.toString;
myfunc2.toString = Object.prototype.toString;
myfunc3.toString = Object.prototype.toString;


new TestCase( SECTION,  "myfunc2.__proto__",                         Function.prototype,     myfunc2.__proto__ );

new TestCase( SECTION,  "myfunc3.__proto__",                         Function.prototype,     myfunc3.__proto__ );

test();
