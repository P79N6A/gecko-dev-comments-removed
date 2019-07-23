





































var gTestfile = 'regress-472528-02.js';

var BUGNUMBER = 472528;
var summary = 'Do not assert: !fp->blockChain';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);

  jit(true);

  try
  {
    for (let i = 0; i < 4; ++i) {
      for (let j = 0; j < 2; ++j) { }
      let (x) (function(){});
    }
  }
  catch(ex)
  {
  }
  jit(false);

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
