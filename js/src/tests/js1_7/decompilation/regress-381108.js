




































var gTestfile = 'regress-381108.js';


var BUGNUMBER = 381108;
var summary = 'decompilation of object literal should have space following :';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  var f = (function() { return {a:3, b getter: f} });
  expect = true;
  actual = /a: 3, b getter: f/.test(f + '');

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
