




































var bug = 354750;
var summary = 'Changing Iterator.prototype.next should not affect default iterator';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber (bug);
  printStatus (summary);
  
  Iterator.prototype.next = function() { 
    throw "This should not be thrown"; 
  }

  expect = 'No exception';
  actual = 'No exception';
  try
  {
    for (var i in [])
    {
    }
  }
  catch(ex)
  {
    actual = ex;
  }
  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
