














var SECTION = "LexicalConventions/regexp-literals-001.js";
var VERSION = "ECMA_2";
var TITLE   = "Regular Expression Literals";

startTest();




s = 

  "passed";

AddTestCase(
  "// should be a comment, not a regular expression literal",
  "passed",
  String(s));

AddTestCase(
  "// typeof object should be type of object declared on following line",
  "passed",
  (typeof s) == "string" ? "passed" : "failed" );

AddTestCase(
  "// should not return an object of the type RegExp",
  "passed",
  (typeof s == "object") ? "failed" : "passed" );

test();
