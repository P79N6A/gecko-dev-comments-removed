




































var gTestfile = 'regress-350810.js';

var BUGNUMBER = 350810;
var summary = 'decompilation for "let" in lvalue part of for..in';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  var f = function () { for ((let (x = 3) y)[5] in []) { } }
  actual = f + '';
  expect = 'function () {\n    for ((let (x = 3) y)[5] in []) {\n    }\n}';
  compareSource(expect, actual, summary);

  exitFunc ('test');
}
