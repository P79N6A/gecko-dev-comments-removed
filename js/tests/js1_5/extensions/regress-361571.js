




































var bug = 361571;
var summary = 'Assertion: fp->scopeChain == parent';
var actual = 'No Crash';
var expect = 'No Crash';


test();


function test()
{
  enterFunc ('test');
  printBugNumber (bug);
  printStatus (summary);
  
  o = {};
  o.__defineSetter__('y', eval); 
  o.watch('y', function () { return "";}); 
  o.y = 1;

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
