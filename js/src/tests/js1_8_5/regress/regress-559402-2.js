



var expect = undefined;
var actual = (function foo() { "bogus"; })();

reportCompare(expect, actual, "ok");
