




































var gTestfile = 'regress-322430.js';

var BUGNUMBER = 322430;
var summary = 'Remove deprecated with statement warning';
var actual = '';
var expect = '';

printBugNumber(BUGNUMBER);
printStatus (summary);

options('strict');
options('werror');

expect = 'No Warning';

try
{
  var obj = {foo: 'baz'};
 
  
  
  
  
  eval('with (obj) { foo; }');

  actual = 'No Warning';
}
catch(ex)
{
  actual = ex + '';
}

reportCompare(expect, actual, summary);
