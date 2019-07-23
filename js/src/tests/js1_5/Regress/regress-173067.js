





































var gTestfile = 'regress-173067.js';

var BUGNUMBER = 173067;
var summary = 'Properly report / in a literal regexp class as an error';
var actual = '';
var expect = 'SyntaxError: unterminated character class ';

printBugNumber(BUGNUMBER);
printStatus (summary);

try
{ 
  var re = eval('/[/]/');
}
catch(e)
{
  actual = e.toString();
}

reportCompare(expect, actual, summary);
