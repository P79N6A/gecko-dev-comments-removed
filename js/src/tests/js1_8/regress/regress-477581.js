






var gTestfile = 'regress-477581.js';

var BUGNUMBER = 477581;
var summary = 'Do not assert: !JSVAL_IS_PRIMITIVE(regs.sp[-2])';
var actual = '';
var expect = '';


test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);

  jit(true);
 
  function g() { yield 2; }
  var iterables = [[1], [], [], [], g()];
  for (let i = 0; i < iterables.length; i++)
    for each (let j in iterables[i])
               ;

  jit(false);

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
