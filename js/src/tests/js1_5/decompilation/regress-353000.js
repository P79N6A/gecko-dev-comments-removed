




































var gTestfile = 'regress-353000.js';

var BUGNUMBER = 353000;
var summary = 'decompilation of RegExp literal after catch block';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);

  var f;
  f = function() { try{}catch(e){} (/g/.x) }
  expect = 'function() { try{}catch(e){} /g/.x; }';
  actual = f + '';
  compareSource(expect, actual, summary);

  exitFunc ('test');
}
