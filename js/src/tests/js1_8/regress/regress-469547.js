




































var gTestfile = 'regress-469547.js';

var BUGNUMBER = 469547;
var summary = 'Do not crash with: for each (let [,] in [[], [], null]) {}';
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
    for each (let [,] in [[], [], null]) {}
  }
  catch(ex)
  {
    print(ex + '');
  }

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
