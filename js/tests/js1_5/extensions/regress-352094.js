




































var bug = 352094;
var summary = 'Do not crash with invalid setter usage';
var actual = 'No Crash';
var expect = 'SyntaxError: invalid setter usage';



test();


function test()
{
  enterFunc ('test');
  printBugNumber (bug);
  printStatus (summary);
  
  try
  {
    eval('(function(){ this.p setter = 0 })()');
  }
  catch(ex)
  {
    actual = ex + '';
  }

  reportCompare(expect, actual, summary);

  try
  {
    eval('(function(){ this.p setter = 0 })()');
  }
  catch(ex)
  {
    actual = ex + '';
  }

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
