




































var gTestfile = 'regress-320032.js';

var BUGNUMBER = 320032;
var summary = 'Parenthesization should not dereference ECMA Reference type';
var actual = 'No error';
var expect = 'No error';

printBugNumber(BUGNUMBER);
printStatus (summary);

if (typeof document != 'undefined' && 'getElementById' in document)
{
  try
  {
    (document.getElementById)("x");
  }
  catch(ex)
  {
    actual = ex + '';
  }
}
 
reportCompare(expect, actual, summary);
