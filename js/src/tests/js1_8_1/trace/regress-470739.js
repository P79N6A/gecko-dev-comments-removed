




































var gTestfile = 'regress-470739.js';

var BUGNUMBER = 470739;
var summary = 'TM: never abort on ==';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);

  function loop()
  {
    var i;
    var start = new Date();

    for(i=0;i<500000;++i) { var r = (void 0) == null; }

    var stop = new Date();
    return stop - start;
  }

  jit(false);
  var timenonjit = loop();
  jit(true);
  var timejit = loop();
  jit(false);

  print('time: nonjit = ' + timenonjit + ', jit = ' + timejit);

  expect = true;
  actual = timejit < timenonjit;

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
