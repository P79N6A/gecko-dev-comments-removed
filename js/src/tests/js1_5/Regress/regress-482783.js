





































var BUGNUMBER = 482783;
var summary = 'TM: Do not crash @ js_ConcatStrings';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);

  jit(true);

  [].concat();
  for (var x = 0; x < 3; ++x) { (null + [,,]); }

  jit(false);

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}

