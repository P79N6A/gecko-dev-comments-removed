




































var gTestfile = 'regress-366668-01.js';

var BUGNUMBER = 366668;
var summary = 'decompilation of "for (let x in x.p)" ';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  var f;

  f = function() { for(let x in x.p) { } };
  expect = 'function() { for(let x in x.p) { } }';
  actual = f + '';
  compareSource(expect, actual, summary);

  exitFunc ('test');
}
