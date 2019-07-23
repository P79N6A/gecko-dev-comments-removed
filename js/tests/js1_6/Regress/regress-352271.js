




































var bug = 352271;
var summary = 'Do not crash with |getter| |for each|';
var actual = 'No Crash';
var expect = 'No Crash';



test();


function test()
{
  enterFunc ('test');
  printBugNumber (bug);
  printStatus (summary);
  
  try
  {
    eval('[window.x getter= t for each ([*].a(v) in [])]');
  }
  catch(ex)
  {
  }

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
