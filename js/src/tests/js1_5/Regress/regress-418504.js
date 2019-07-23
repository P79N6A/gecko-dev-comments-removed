




































var gTestfile = 'regress-418504.js';

var BUGNUMBER = 418504;
var summary = 'Untagged boolean stored in a jsval in JS_ConvertValue';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  expect = false;
  actual = RegExp.multiline;
  reportCompare(expect, actual, 'RegExp.multiline');

  expect = true;
  RegExp.multiline = 17;
  actual = RegExp.multiline;
  reportCompare(expect, actual, 'RegExp.multiline = 17');

  expect = true;
  RegExp.multiline = 17;
  actual = RegExp.multiline;
  reportCompare(expect, actual, 'RegExp.multiline = 17');

  expect = true;
  RegExp.multiline = 17;
  actual = RegExp.multiline;
  reportCompare(expect, actual, 'RegExp.multiline = 17');

  expect = true;
  RegExp.multiline = true;
  actual = RegExp.multiline;
  reportCompare(expect, actual, 'RegExp.multiline = true');

  expect = true;
  RegExp.multiline = 17;
  actual = RegExp.multiline;
  reportCompare(expect, actual, 'RegExp.multiline = 17');

  exitFunc ('test');
}
