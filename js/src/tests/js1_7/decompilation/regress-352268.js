




































var gTestfile = 'regress-352268.js';

var BUGNUMBER = 352268;
var summary = 'decompilation should not change scope of |let| in |else|...|if|';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);

  var c;
  var f;

  try
  {
    c = '(function() { if(x) { } else if (y) let b=2; })';
    f = eval(c);
    expect = 'function() { if(x) { } else if (y) let b=2; }';
    actual = f + '';
    compareSource(expect, actual, summary);
  }
  catch(ex)
  {
    
    summary = 'let declaration must be direct child of block or top-level implicit block';
    expect = 'SyntaxError';
    actual = ex.name;
    reportCompare(expect, actual, summary);
  }

  exitFunc ('test');
}
