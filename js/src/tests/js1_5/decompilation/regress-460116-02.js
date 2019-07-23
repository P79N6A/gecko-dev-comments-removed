




































var gTestfile = 'regress-460116-02.js';

var BUGNUMBER = 460116;
var summary = 'Condition propagation folding constants';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);

  var f;

  f = (function (){if((false&&foo)===(true||bar));});
  expect = 'function (){if((false&&foo)===(true||bar)) {}}';
  actual = f + '';

  compareSource(expect, actual, summary);

  exitFunc ('test');
}
