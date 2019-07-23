




































var gTestfile = 'regress-469927.js';

var BUGNUMBER = 469927;
var summary = 'TM: jit should not slow down short loop with let';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  function letitbe() {
    var start = new Date();
    for (let i = 0; i < 500000; ++i) { 
      for (let j = 0; j < 4; ++j) { } 
    }
    var stop = new Date();
    return stop - start;
  }

  jit(false);
  var timenonjit = letitbe();
  jit(true);
  var timejit = letitbe();
  jit(false);

  print('time: nonjit = ' + timenonjit + ', jit = ' + timejit);

  expect = true;
  actual = timejit < timenonjit/2;

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
