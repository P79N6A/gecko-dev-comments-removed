







var BUGNUMBER = 477581;
var summary = 'Do not assert: !regs.sp[-2].isPrimitive()';
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
