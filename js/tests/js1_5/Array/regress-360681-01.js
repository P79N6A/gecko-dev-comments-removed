




































var bug = 360681;
var summary = 'Regression from bug 224128';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber (bug);
  printStatus (summary);
  
  expect = actual = 'No Crash';

  var a = Array(3);
  a[0] = 1;
  a[1] = 2;
  a.sort(function () { gc(); return 1; });

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
