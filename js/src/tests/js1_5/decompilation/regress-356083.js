





































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
 
  var f = function() { return { set this() { } }; } ;
  expect = 'function() { return { set this() { } }; }';
  actual = f + '';

  compareSource(expect, actual, summary);

  expect = "({ set ''() {} })";
  actual = uneval({ set ''() {} });
  compareSource(expect, actual, expect);
  exitFunc ('test');
}
