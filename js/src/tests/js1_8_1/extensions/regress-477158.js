




































var gTestfile = 'regress-477158.js';

var BUGNUMBER = 477158;
var summary = 'Do not assert: v == JSVAL_TRUE || v == JSVAL_FALSE';
var actual = '';
var expect = '';


test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);

  jit(true);

  x = 0;
  x = x.prop;
  for each (let [] in ['', '']) { switch(x) { default: (function(){}); } };

  jit(false);

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
