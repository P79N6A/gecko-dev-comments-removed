




































var gTestfile = 'regress-349605.js';

var BUGNUMBER = 349605;
var summary = 'decompilation of let inside |for| statements';
var actual = '';
var expect = '';


test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);

  var f = function()
    { alert(1); for((let(y=3) let(y=4) y); 0; x++) ; alert(6); }

  expect = 'function () {\n    alert(1);\n' +
    '    for ((let (y = 3) (let (y = 4) y)); 0; x++) {\n' +
    '    }\n' +
    '    alert(6);\n' +
    '}';

  actual = f + '';

  compareSource(expect, actual, summary);

  exitFunc ('test');
}
