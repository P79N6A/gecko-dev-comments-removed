




































var gTestfile = 'regress-352455.js';

var BUGNUMBER = 352455;
var summary = 'Eval object with non-function getters/setters';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);

  print('If the test harness fails on this bug, the test fails.');
 
  expect = 'SyntaxError: invalid getter usage';
  z = ({});
  try { eval('z.x getter= /g/i;'); } catch(ex) { actual = ex + '';}
  print("This line should not be the last output you see.");
  try { print(uneval(z)); } catch(e) { print("Threw!"); print(e); }

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
