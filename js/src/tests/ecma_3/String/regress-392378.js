




































var gTestfile = 'regress-392378.js';

var BUGNUMBER = 392378;
var summary = 'Regular Expression Non-participating Capture Groups are inaccurate in edge cases';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  expect = ["", undefined, ""] + '';
  actual = "y".split(/(x)?\1y/) + '';
  reportCompare(expect, actual, summary + ': "y".split(/(x)?\1y/)');

  expect = ["", undefined, ""] + '';
  actual = "y".split(/(x)?y/) + '';
  reportCompare(expect, actual, summary + ': "y".split(/(x)?y/)');

  expect = 'undefined';
  actual = "y".replace(/(x)?\1y/, function($0, $1){ return String($1); }) + '';
  reportCompare(expect, actual, summary + ': "y".replace(/(x)?\\1y/, function($0, $1){ return String($1); })');

  expect = 'undefined';
  actual = "y".replace(/(x)?y/, function($0, $1){ return String($1); }) + '';
  reportCompare(expect, actual, summary + ': "y".replace(/(x)?y/, function($0, $1){ return String($1); })');

  expect = 'undefined';
  actual = "y".replace(/(x)?y/, function($0, $1){ return $1; }) + '';
  reportCompare(expect, actual, summary + ': "y".replace(/(x)?y/, function($0, $1){ return $1; })');

  exitFunc ('test');
}
