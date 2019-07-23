




































var gTestfile = 'regress-465136.js';

var BUGNUMBER = 465136;
var summary = 'false == ""';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  expect = 'true,true,true,true,true,';
  actual = '';

  jit(true);

  for (var i=0;i<5;++i) actual += (false == '') + ',';

  jit(false);

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
