




































var gTestfile = 'regress-460024.js';

var BUGNUMBER = 460024;
var summary = 'Regression from bug 451154';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);

  expect = 'PASS';
  actual = 'FAIL';

  jit(true);

  var js = 'Function.prototype.inherits = function(a) {' +
  '  actual = "PASS";' +
  '};' +
  'function f() { }' +
  'f.inherits();';
  function doeval(callback) { callback(js) };
  doeval(eval);

  jit(false);

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
