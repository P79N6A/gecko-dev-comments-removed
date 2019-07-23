




































var gTestfile = 'regress-488989.js';

var BUGNUMBER = 488989;
var summary = 'Array.prototype.push for non-arrays near max-array-index limit';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);

  var stack = { push: [].push }; stack.length = Math.pow(2, 37);
  stack.push(-2, -1, 0);

  var stack = { push: [].push }; stack.length = Math.pow(2, 5);
  stack.push(-2, -1, 0);

  var stack = { push: [].push }; stack.length = Math.pow(2, 32) -2;
  stack.push(-2, -1, 0);

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
