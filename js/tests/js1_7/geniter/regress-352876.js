




































var bug = 352876;
var summary = 'Do not assert with nested finally return|yield';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber (bug);
  printStatus (summary);
  
  expect = '[object StopIteration]';
  actual = '';
  try
  {
    var g = (function() { 
        try { try { } finally { return; } } finally { yield 3; }
      })();

    g.next();
    g.next();
  }
  catch(ex)
  {
    actual = ex + '';
  }

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
