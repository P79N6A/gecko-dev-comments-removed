




































var gTestfile = 'regress-352392.js';

var BUGNUMBER = 352392;
var summary = 'Do not hang/crash |for each| over object with getter set to map';
var actual = 'No Crash';
var expect = 'No Crash';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);

  expect = 'SyntaxError: invalid for each loop';
  try
  {
    var obj = { };
    obj.y getter = Array.prototype.map;
    eval('(function() { for each(let z in obj) { } })()');
  }
  catch(ex)
  {
    actual = ex + '';
  }

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
