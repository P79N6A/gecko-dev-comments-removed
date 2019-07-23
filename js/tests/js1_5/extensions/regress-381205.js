




































var gTestfile = 'regress-381205.js';


var BUGNUMBER = 381205;
var summary = 'uneval with special getter functions';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  expect = 'SyntaxError: invalid getter usage';

  getter function p() { print(4) }
  try
  {
    uneval({x getter: this.__lookupGetter__("p")});
  }
  catch(ex)
  {
    actual = ex + '';
  }
  
  reportCompare(expect, actual, summary);


  exitFunc ('test');
}
