




































var gTestfile = 'regress-465483.js';

var BUGNUMBER = 465483;
var summary = 'Type instability leads to undefined being added as a string instead of as a number';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  expect = 'NaN';

  jit(true);
  for each (i in [4, 'a', 'b', (void 0)]) print(actual = '' + (i + i));
  jit(false);

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
