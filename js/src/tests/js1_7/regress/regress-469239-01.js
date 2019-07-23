




































var gTestfile = 'regress-469239-01.js';

var BUGNUMBER = 469239;
var summary = 'TM: Do not assert: entry->kpc == (jsbytecode*) atoms[index]';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);

  jit(true);

  for (let b=0;b<9;++b) {
    for each (let h in ['', 3, /x/]) {
	    for each (c in [[], [], [], /x/]) {
        '' + c;
	    }
    }
  }

  jit(false);

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
