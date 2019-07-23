




































var gTestfile = 'regress-349851.js';

var BUGNUMBER = 349851;
var summary = 'decompilation of yield \\n, 3';
var actual = '';
var expect = 'SyntaxError';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  
  try
  {
    var f = eval('function(){ yield \n,3 }');
  }
  catch(ex)
  {
    actual = ex.name;
  }

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
