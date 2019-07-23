




































var gTestfile = 'regress-352870-03.js';

var BUGNUMBER = 352870;
var summary = 'Do not assert for crazy huge testcases';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  expect = /ReferenceError: (k|"k") is not defined/;
  actual = '';
  try
  {
    (function() { switch(3) {
       case ([<{z}></{z}>.([[1]])
	      for (x in ([j=k for (y in [1])]))]): } })();
  }
  catch(ex)
  {
    actual = ex + '';
  }
  reportMatch(expect, actual, summary);

  exitFunc ('test');
}
