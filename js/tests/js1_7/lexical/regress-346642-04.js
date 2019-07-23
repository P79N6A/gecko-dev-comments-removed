





































var bug = 346642;
var summary = 'decompilation of destructuring assignment';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber (bug);
  printStatus (summary);
  
  expect = 'No Crash';
  actual = 'No Crash';
  try
  {
    function() { for (var [a, b] in []) for ([c, d] in []) { } }
  }
  catch(ex)
  {
    actual = ex + '';
  }
  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
