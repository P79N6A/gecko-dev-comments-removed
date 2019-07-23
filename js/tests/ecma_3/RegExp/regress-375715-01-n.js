




































var gTestfile = 'regress-375715-01-n.js';

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
 
  
  
  

  /[\Wb-G]/.exec("");
  reportCompare(expect, actual, summary + ' /[\Wb-G]/.exec("")');

  exitFunc ('test');
}
