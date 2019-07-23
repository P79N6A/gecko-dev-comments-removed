






































var gTestfile = 'regress-349648.js';

var BUGNUMBER = 349648;
var summary = 'Extra "[" in decomilation of nested array comprehensions';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  var f;

  f = function(){ [[0 for (x in [])] for (y in []) ]; };
  expect = 'function () {\n    [[0 for (x in [])] for (y in [])];\n}';
  actual = f + '';
  reportCompare(expect, actual, summary + ':1');

  f = function(){ [[0 for (x in [])] for (y in []) ]; };
  expect = 'function () {\n    [[0 for (x in [])] for (y in [])];\n}';
  actual = f + '';
  reportCompare(expect, actual, summary + ':2');

  f = function(){ [[0 for (x in [])] for (yyyyyyyyyyy in []) ]; }
  expect = 'function () {\n    [[0 for (x in [])] for (yyyyyyyyyyy in [])];\n}';
  actual = f + '';
  reportCompare(expect, actual, summary + ':3');

  f = function(){ [0 for (x in [])]; }
  expect = 'function () {\n    [0 for (x in [])];\n}';
  actual = f + '';
  reportCompare(expect, actual, summary + ':4');

  f = function(){ [[[0 for (x in [])] for (yyyyyyyyyyy in []) ]
                   for (zzz in [])]; }
  expect = 'function () {\n    [[[0 for (x in [])] for (yyyyyyyyyyy in [])]' +
    ' for (zzz in [])];\n}';
  actual = f + '';
  reportCompare(expect, actual, summary + ':5');

  f = function(){ [[0 for (x in [])] for (y in []) ]; }
  expect = 'function () {\n    [[0 for (x in [])] for (y in [])];\n}';
  actual = f + '';
  reportCompare(expect, actual, summary + ':6');

  f = function(){ [[11 for (x in [])] for (y in []) ]; }
  expect = 'function () {\n    [[11 for (x in [])] for (y in [])];\n}';
  actual = f + '';
  reportCompare(expect, actual, summary + ':7');

  exitFunc ('test');
}
