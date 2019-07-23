




































var gTestfile = 'regress-375882.js';

var BUGNUMBER = 375882;
var summary = 'Decompilation of switch with case 0/0';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  var f = function()
    {
      switch(a)
      {
        case 0/0: a;
        case 1/0: b;
        case -1/0: c;
        case -0:
        d;
      }
    };

  expect = 'function ( ) { switch ( a ) { case 0 / 0 : a ; case 1 / 0 : b ; ' +
    'case 1 / - 0 : c ; case - 0 : d ; default : ; } } ';

  actual = f + '';

  compareSource(expect, actual, summary);

  exitFunc ('test');
}
