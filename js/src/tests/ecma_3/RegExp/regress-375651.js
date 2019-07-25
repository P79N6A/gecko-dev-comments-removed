






































var BUGNUMBER = 375651;
var summary = 'Do not assert with regexp quantifiers';
var actual = 'No Crash';
var expect = 'No Crash';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  /(.{2,3}){0,2}?t/.exec("abt");

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
