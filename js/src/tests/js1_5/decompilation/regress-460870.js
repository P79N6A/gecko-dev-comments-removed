




































var gTestfile = 'regress-460870.js';

var BUGNUMBER = 460870;
var summary = 'Decompilation of (function() { if (a || 1 || 2) { } })';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  expect = 'function ( ) { if ( a || true ) { } }';
  var f = (function() { if (a || 1 || 2) { } });
  actual = f + '';

  compareSource(expect, actual, summary);

  exitFunc ('test');
}
