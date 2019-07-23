




































var gTestfile = 'regress-416737-02.js';

var BUGNUMBER = 416737;
var summary = 'Do not assert: *pc == JSOP_GETARG';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);

  var f = function([]){ function n(){} };
  if (typeof dis == 'function')
  {
    dis(f);
  }
  print(f);

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
