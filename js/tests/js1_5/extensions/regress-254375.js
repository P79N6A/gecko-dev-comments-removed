





































var gTestfile = 'regress-254375.js';

var BUGNUMBER = 254375;
var summary = 'Object.toSource for negative number property names';
var actual = '';
var expect = '';

printBugNumber(BUGNUMBER);
printStatus (summary);
 
if (typeof uneval != 'undefined')
{
  try
  {
    expect = 'no error';
    eval(uneval({'-1':true}));
    actual = 'no error';
  }
  catch(e)
  {
    actual = 'error';
  }

  reportCompare(expect, actual, summary);
}
