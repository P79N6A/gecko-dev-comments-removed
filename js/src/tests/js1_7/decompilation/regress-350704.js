




































var gTestfile = 'regress-350704.js';

var BUGNUMBER = 350704;
var summary = 'decompilation of let nested in for';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);

  var f;

  f = function() { try{} catch(y) { for(z(let(y=3)4); ; ) ; } }
  expect = 'function () {\n    try {\n    } catch (y) {\n        ' +
    'for (z(let (y = 3) 4);;) {\n        }\n    }\n}'
    actual = f + '';
  compareSource(expect, actual, summary);

  exitFunc ('test');
}
