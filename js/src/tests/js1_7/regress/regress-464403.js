





































var gTestfile = 'regress-464403.js';

var BUGNUMBER = 464403;
var summary = 'Do not assert: tm->recorder != NULL';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  jit(true);

  print(8);
  var u = [print, print, function(){}];
  for each (x in u) for (u.e in [1,1,1,1]);

  jit(false);

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
