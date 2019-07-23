




































var gTestfile = 'regress-375715-02.js';

var BUGNUMBER = 375715;
var summary = 'Do not assert: (c2 <= cs->length) && (c1 <= c2)';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  /[\s-:]/;
  reportCompare(expect, actual, summary + '/[\s-:]/');

  exitFunc ('test');
}
