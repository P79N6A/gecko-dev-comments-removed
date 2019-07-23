




































var gTestfile = 'regress-352732.js';

var BUGNUMBER = 352732;
var summary = 'Decompiling "if (x) L: let x;"';
var summarytrunk = 'let declaration must be direct child of block or top-level implicit block';
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
    c = '(function() { if (x) L: let x; })';
    f = eval(c);
    expect = 'function() { if (x) { L: let x; } }';
    actual = f + '';
    compareSource(expect, actual, summary);
  }
  catch(ex)
  {
    
    expect = 'SyntaxError';
    actual = ex.name;
    reportCompare(expect, actual, summarytrunk + ': ' + c);
  }

  try
  {
    c = '(function() { if (x) L: let x; else y; })';
    f = eval(c);
    expect = 'function() { if (x) { L: let x; } else { y;} }';
    actual = f + '';
    compareSource(expect, actual, summary);
  }
  catch(ex)
  {
    
    expect = 'SyntaxError';
    actual = ex.name;
    reportCompare(expect, actual, summarytrunk + ': ' + c);
  }

  exitFunc ('test');
}
