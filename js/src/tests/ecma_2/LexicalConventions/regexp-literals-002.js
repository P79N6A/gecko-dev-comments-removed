













var SECTION = "LexicalConventions/regexp-literals-002.js";
var VERSION = "ECMA_2";
var TITLE   = "Regular Expression Literals";

startTest();



AddTestCase(
  "// A regular expression literal represents an object of type RegExp.",
  "true",
  (/x*/ instanceof RegExp).toString() );

test();
