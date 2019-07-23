




































var bug = 368859;
var summary = 'large sharp variable numbers should not be rounded down.';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber (bug);
  printStatus (summary);
  
  expect = 'SyntaxError: overlarge sharp variable number';

  try
  {
    print(eval('(function(){ return #65535#, #65536#; })'));
  }
  catch(ex)
  {
    actual = ex + '';
  }

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
