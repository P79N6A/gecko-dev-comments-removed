




































var gTestfile = 'regress-375715-04.js';

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
 
  (new RegExp("[\xDF-\xC7]]", "i")).exec("");
  reportCompare(expect, actual, summary + '(new RegExp("[\xDF-\xC7]]", "i")).exec("")');

  exitFunc ('test');
}
