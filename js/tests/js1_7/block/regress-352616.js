




































var gTestfile = 'regress-352616.js';

var BUGNUMBER = 352616;
var summary = 'Do not Crash reporting error with |for..in| and |let|';
var actual = 'No Crash';
var expect = 'No Crash';


test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);

  expect = 'TypeError: (let (b = 1) 2).c is not a function';
  actual = 'No Error';
  try
  { 
    for(a in (let (b=1) 2).c(3)) { };
  }
  catch(ex)
  {
    actual = ex + '';
  }

  reportCompare(expect, actual, summary + ': 1');

  expect = 'TypeError: (let (b = 1, d = 2) 2).c is not a function';
  actual = 'No Error';
  try
  { 
    for(a in (let (b=1,d=2) 2).c(3)) { };
  }
  catch(ex)
  {
    actual = ex + '';
  }

  reportCompare(expect, actual, summary + ': 2');

  expect = 'TypeError: (let (b = 1, d = 2) 2).c is not a function';
  actual = 'No Error';
  try
  { 
    for(a in (let (b=1,d=2) 2).c(3)) { };
  }
  catch(ex)
  {
    actual = ex + '';
  }

  reportCompare(expect, actual, summary + ': 3');

  exitFunc ('test');
}
