





































var BUGNUMBER = 356083;
var summary = 'decompilation for (function () {return {set this(v) {}};}) ';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  var f = function() { return { set this(v) { } }; } ;
  expect = 'function() { return { set this(v) { } }; }';
  actual = f + '';

  compareSource(expect, actual, summary);

  expect = "({ set ''(v) {} })";
  actual = uneval({ set ''(v) {} });
  compareSource(expect, actual, expect);
  exitFunc ('test');
}
