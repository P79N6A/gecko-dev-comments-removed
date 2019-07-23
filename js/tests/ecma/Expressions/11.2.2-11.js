





































gTestfile = '11.2.2-11.js';












































var SECTION = "11.2.2-9-n.js";
var VERSION = "ECMA_1";
startTest();
var TITLE   = "The new operator";

writeHeaderToLog( SECTION + " "+ TITLE);

var FUNCTION = new Function();

new TestCase( SECTION,
              "var FUNCTION = new Function(); f = new FUNCTION(); typeof f",
              "object",
              eval("var FUNCTION = new Function(); f = new FUNCTION(); typeof f") );

new TestCase( SECTION,
              "var FUNCTION = new Function('return this'); f = new FUNCTION(); typeof f",
              "object",
              eval("var FUNCTION = new Function('return this'); f = new FUNCTION(); typeof f") );

test();

