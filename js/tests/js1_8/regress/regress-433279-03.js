




































var gTestfile = 'regress-433279-03.js';

var BUGNUMBER = 433279;
var summary = 'Do not assert: pn != tc->parseContext->nodeList';
var actual = 'No Crash';
var expect = 'No Crash';


test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  try
  {
    eval('({a}) = b; with({}) { for(let y in z) { } }');
  }
  catch(ex)
  {
  }

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
