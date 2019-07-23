




































var gTestfile = 'regress-356378.js';

var BUGNUMBER = 356378;
var summary = 'var x; x getter= function () { };';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);

  expect = 'SyntaxError: invalid getter usage';
  try
  {
    eval('(function() { var x; x getter= function () { }; })();');
  }
  catch(ex)
  {
    actual = ex + '';
  }

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
