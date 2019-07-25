





































var BUGNUMBER = 355736;
var summary = 'Decompilation of "[reserved]" has extra quotes';
var actual = '';
var expect = '';
var f;


test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  f = function() { [implements] = q; };
  expect = 'function() { [implements] = q; }';
  actual = f + '';
  compareSource(expect, actual, summary + ': 1');

  f = function() { return { get implements() { } } };
  expect = 'function() { return { get implements() { } }; }';
  actual = f + '';
  compareSource(expect, actual, summary + ': 2');

  f = function() { [goto] = a };
  expect = 'function() { [goto] = a; }';
  actual = f + '';
  compareSource(expect, actual, summary + ': 3');

  exitFunc ('test');
}
