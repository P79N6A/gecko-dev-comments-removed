





































var gTestfile = 'regress-350991.js';

var BUGNUMBER = 350991;
var summary = 'decompilation of function () { for (let...;...;}} ';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);

  var f;

  expect = 'function () {\n    for ((let (y) 3);;) {\n    }\n}';
  try
  {
    f = eval('(function () { for ((let (y) 3); ;) { } })');
    actual = f + '';
  }
  catch(ex)
  {
    actual = ex + '';
  }

  compareSource(expect, actual, summary);

  expect = 'function () {\n    let x = 5;\n    while (x-- > 0) {\n' +
    '        for (let x = x, q = 5;;) {\n        }\n    }\n}';
  try
  {
    f = function() { let x = 5; while (x-- > 0) { for (let x = x, q = 5;;); } }
    actual = f + '';
  }
  catch(ex)
  {
    actual = ex + '';
  }

  compareSource(expect, actual, summary);

  exitFunc ('test');
}
