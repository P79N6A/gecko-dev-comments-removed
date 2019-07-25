







var BUGNUMBER = 530537;
var summary = 'Decompilation of JSOP_CONCATN with a ternary operator';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);

  
  var f = function (b) { dump((b ? "yes" : "no") + "_bye"); }
  expect = 'function (b) { dump((b ? "yes" : "no") + "_bye"); }';
  actual = f + '';

  compareSource(expect, actual, summary.replace("ternary", "plus"));

  var f = function (b) { dump("hi_" + (b ? "yes" : "no") + "_bye"); }
  expect = 'function (b) { dump("hi_" + (b ? "yes" : "no") + "_bye"); }';
  actual = f + '';

  compareSource(expect, actual, summary);

  exitFunc ('test');
}

