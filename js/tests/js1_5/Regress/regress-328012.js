




































var gTestfile = 'regress-328012.js';

var BUGNUMBER = 328012;
var summary = 'Content PropertyIterator should not root in chrome';
var actual = 'No Error';
var expect = 'No Error';


printBugNumber(BUGNUMBER);
printStatus (summary);

if (typeof focus != 'undefined')
{
  try
  {
    for (prop in focus.prototype.toString)
      ;
  }
  catch(ex)
  {
    printStatus(ex + '');
    actual = ex + '';
  }
}
reportCompare(expect, actual, summary);
