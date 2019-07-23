




































var gTestfile = 'regress-422348.js';

var BUGNUMBER = 422348;
var summary = 'Proper overflow error reporting';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  expect = 'InternalError: allocation size overflow';
  try 
  { 
    Array(1 << 30).sort(); 
    actual = 'No Error';
  } 
  catch (ex) 
  { 
    actual = ex + '';
  } 

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
