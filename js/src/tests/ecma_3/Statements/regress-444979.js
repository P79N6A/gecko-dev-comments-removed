






































var BUGNUMBER = 444979;
var summary = 'switch -0 is same as switch 0';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  expect = 'y==0';
  actual = '';

  var shortSwitch = 'var y=-0;switch (y){case 0: actual="y==0";  break; default: actual="y!=0";}';
  eval(shortSwitch);

  reportCompare(expect, actual, summary + ': shortSwitch');

  actual = '';

  var longSwitch = 'var y=-0;var t=0;switch(y){case -1:';
  for (var i = 0; i < 64000; i++) {
    longSwitch += ' t++;';
  }
  longSwitch += ' break; case 0: actual = "y==0"; break; default: actual = "y!=0";}';
  eval(longSwitch);

  reportCompare(expect, actual, summary + ': longSwitch');

  exitFunc ('test');
}
