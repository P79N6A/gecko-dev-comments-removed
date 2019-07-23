




































var gTestfile = 'regress-404734.js';

var BUGNUMBER = 404734;
var summary = 'Object destructuring shorthand';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  var o = {p: 42, q: true};
  var {p, q} = o;

  expect = '42,true';
  actual = p + ',' + q;
  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
