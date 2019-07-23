




































var gTestfile = 'regress-379482.js';


var BUGNUMBER = 379482;
var summary = 'Decompilation of float setter';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  var f = function() { ({ 1.1 setter: 2 }) };

  expect = 'function() { ({ 1.1 setter: 2 }); }';
  actual = f + '';
  compareSource(expect, actual, summary);

  exitFunc ('test');
}
