




































var gTestfile = 'regress-469234.js';

var BUGNUMBER = 469234;
var summary = 'TM: Do not assert: !JS_ON_TRACE(cx)';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);

  jit(true);

  for(var j=0;j<3;++j)({__proto__:[]}).__defineSetter__('x',function(){});

  jit(false);

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
