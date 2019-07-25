





































var BUGNUMBER = 472533;
var summary = 'Do not crash with loop, replace, regexp';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  for (var j = 0; j < 4; ++j) ''.replace('', /x/);

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
