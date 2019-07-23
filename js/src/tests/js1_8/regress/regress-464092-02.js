




































var gTestfile = 'regress-464092-02.js';

var BUGNUMBER = 464092;
var summary = 'Censor block objects';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);

  try
  {
    let (a) 'b'.replace(/b/g, function() c = this );
  }
  catch(ex)
  {
    actual = ex + '';
  }

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
