




































var gTestfile = 'regress-452884-01.js';

var BUGNUMBER = 452884;
var summary = 'Do not crash in switch';
var actual = 'No Crash';
var expect = 'No Crash';


test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);

  if (!options().match(/jit/))
  {
    options('jit');
  }

  for (var j=0;j<5;++j) { switch(1.1) { case NaN: case 2: } }
 
  options('jit');

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
