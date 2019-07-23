




































var gTestfile = 'regress-376052.js';

var BUGNUMBER = 376052;
var summary = 'javascript.options.anonfunfix to allow function (){} expressions';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);

  if (typeof window != 'undefined')
  {
    print('Test skipped. anonfunfix not configurable in browser.');
    reportCompare(expect, actual, summary);
  }
  else
  {
    expect = 'No Error';
    try
    {
      eval('function () {1;}');
      actual = 'No Error';
    }
    catch(ex)
    {
      actual = ex + '';
    }
    reportCompare(expect, actual, summary + ': 1');

    options('anonfunfix');

    expect = 'No Error';
    try
    {
      eval('(function () {1;})');
      actual = 'No Error';
    }
    catch(ex)
    {
      actual = ex + '';
    }
    reportCompare(expect, actual, summary + ': 2');

    expect = 'SyntaxError: syntax error';
    try
    {
      eval('function () {1;}');
      actual = 'No Error';
    }
    catch(ex)
    {
      actual = ex + '';
    }
    reportCompare(expect, actual, summary + ': 3');

  }

  exitFunc ('test');
}
