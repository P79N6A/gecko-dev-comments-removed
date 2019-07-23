




































var gTestfile = 'regress-380933.js';


var BUGNUMBER = 380933;
var summary = 'Do not assert with uneval object with setter with modified proto';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  var f = (function(){}); 
  var y = ({p getter: f}); 
  f.__proto__ = []; 

  expect = /TypeError: Array.prototype.toSource called on incompatible Function/;
  try
  {
    uneval(y);
    actual = 'No Error';
  }
  catch(ex)
  {
    actual = ex + '';
  }

  reportMatch(expect, actual, summary);

  exitFunc ('test');
}
