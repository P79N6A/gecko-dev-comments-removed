




































var gTestfile = 'regress-352217.js';

var BUGNUMBER = 352217;
var summary = 'Need space between |else|, |let|';
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
    c = '(function() { if(g) h; else let x; })';
    f = eval(c);
    expect = 'function() { if(g) { h; } else let x; }';
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
