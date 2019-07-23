




































var gTestfile = 'regress-478047.js';

var BUGNUMBER = 478047;
var summary = 'Assign to property with getter but no setter should throw ' +
  'TypeError';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);

  expect = 'TypeError: setting a property that has only a getter';
  try
  { 
    var o = { get p() { return "a"; } };
    o.p = "b";
  }
  catch(ex)
  {
    actual = ex + '';
  }
  reportCompare(expect, actual, summary);


  actual = '';
  try
  {
    o = { get p() { return "a"; } };
    T = (function () {});
    T.prototype = o;
    y = new T();
    y.p = "b";
  }
  catch(ex)
  {
    actual = ex + '';
  }

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
