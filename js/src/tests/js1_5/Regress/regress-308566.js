




































var gTestfile = 'regress-308566.js';

var BUGNUMBER = 308566;
var summary = 'Do not treat octal sequence as regexp backrefs in strict mode';
var actual = 'No error';
var expect = 'No error';

printBugNumber(BUGNUMBER);
printStatus (summary);
 
options('strict');
options('werror');

try
{
  var c = eval("/\260/");
}
catch(e)
{
  actual = e + '';
}

reportCompare(expect, actual, summary);
