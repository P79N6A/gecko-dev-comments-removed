




































var gTestfile = 'regress-328664.js';

var BUGNUMBER = 328664;
var summary = 'Correct error message for funccall(undefined, undefined.prop)';
var actual = '';
var expect = /TypeError: value.parameters (has no properties|is undefined)/;

printBugNumber(BUGNUMBER);
printStatus (summary);
 
var value = {};

function funccall(a,b)
{
}

try
{
  funccall(value[1], value.parameters[1]);
}
catch(ex)
{
  printStatus(ex);
  actual = ex + '';
}

reportMatch(expect, actual, summary);
