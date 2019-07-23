




































var bug = 356248;
var summary = 'Decompilation of object literal with named getter';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber (bug);
  printStatus (summary);
  
  var f = function() { return {p setter: function y() { } } };
  expect = 'function ( ) { return { set p y ( ) { } } ; } ';
  actual = f + '';

  compareSource(expect, actual, summary);

  exitFunc ('test');
}
