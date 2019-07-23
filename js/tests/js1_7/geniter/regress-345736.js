




































var bug = 345736;
var summary = 'for each in array comprehensions';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber (bug);
  printStatus (summary);

  var arr;

  arr=[x+x for (x in ["a","b","c"])];

  expect = '00,11,22';
  actual = arr.toString();
  reportCompare(expect, actual, summary);

  arr=[x+x for each (x in ["a","b","c"])];

  expect = 'aa,bb,cc';
  actual = arr.toString();
  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
