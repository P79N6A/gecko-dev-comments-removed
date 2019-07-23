





































var bug = 352060;
var summary = 'decompilation of getter, setter revisited';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber (bug);
  printStatus (summary);
  
  var f;

  f = function() { foo setter = function(){} }
  expect = 'function() { foo setter = function(){}; }';
  actual = f + '';
  compareSource(expect, actual, summary);

  f = function() { foo.bar setter = function(){} }
  expect = 'function() { foo.bar setter = function(){}; }';
  actual = f + '';
  compareSource(expect, actual, summary);

  f = function(){ var y = new Array(); y[0] getter = function(){}; } 
  expect = 'function(){ var y = new Array(); y[0] getter = function(){}; } ';
  actual = f + '';
  compareSource(expect, actual, summary);

  f = function(){ var foo = <foo bar="baz"/>; foo.@bar getter = function(){}; }
  expect = 'function(){ var foo = <foo bar="baz"/>; foo.@bar getter = function(){}; }';
  actual = f + '';
  compareSource(expect, actual, summary);

  exitFunc ('test');
}
