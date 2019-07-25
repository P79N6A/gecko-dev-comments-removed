





































var BUGNUMBER = 460886;
var summary = 'Do not asssert: end >= begin';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);

  for (var j = 0; j < 5; ++j) { "".substring(5); }
 
  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
