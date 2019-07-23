




































var gTestfile = 'regress-351496.js';

var BUGNUMBER = 351496;
var summary = 'decompilation of case let (y = 3) expression';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);

  var f = function() { switch(0) { case let(y = 3) 6: } }
  actual = f + '';
  expect = 'function () {\n' +
    '    switch (0) {\n' +
    '      case let (y = 3) 6:\n' +
    '      default:;\n' +
    '    }\n' +
    '}';

  compareSource(expect, actual, summary);

  exitFunc ('test');
}
