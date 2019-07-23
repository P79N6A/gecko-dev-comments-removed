




































var gTestfile = 'regress-356247.js';


var BUGNUMBER = 356247;
var summary = 'Decompilation of let {} = [1] in a loop';
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
  var g;

  try
  { 
    c = '(function() { for(let x in []) let {} = [1]; })';
    f = eval(c);
    expect = 'function() { for(let x in []) let [] = [1]; }';
    actual = f + '';
    compareSource(expect, actual, summary + ': f : ' + c);

    g = eval('(' + f + ')');
    actual = g + '';
    compareSource(expect, actual, summary + ': g : ' + c);
  }
  catch(ex)
  {
    
    expect = 'SyntaxError';
    actual = ex.name;
    reportCompare(expect, actual, summarytrunk + ': ' + c);
  }

  try
  {
    c = '(function() { while(0) let {} = [1]; })';
    f = eval(c);
    expect = 'function() { while(0) let [] = [1]; }';
    actual = f + '';
    compareSource(expect, actual, summary + ': f : ' + c);

    g = eval('(' + f + ')');
    actual = g + '';
    compareSource(expect, actual, summary + ': g : ' + c);
  }
  catch(ex)
  {
    
    expect = 'SyntaxError';
    actual = ex.name;
    reportCompare(expect, actual, summarytrunk + ': ' + c);
  }

  exitFunc ('test');
}
