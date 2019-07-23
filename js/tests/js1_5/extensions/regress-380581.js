




































var gTestfile = 'regress-380581.js';


var BUGNUMBER = 380581;
var summary = 'Incorrect uneval with setter in object literal';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  expect = '({ set x () {}})';
  actual = uneval({x setter: eval("(function () { })") });
  compareSource(expect, actual, summary);
  
  expect = '(function() { })';
  actual = uneval(eval("(function() { })"));
  compareSource(expect, actual, summary);
    
  expect = '(function() { })';
  actual = uneval(eval("(function() { })"));
  compareSource(expect, actual, summary);

  exitFunc ('test');
}
