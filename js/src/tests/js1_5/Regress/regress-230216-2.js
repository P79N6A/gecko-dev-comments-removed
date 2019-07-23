





































var gTestfile = 'regress-230216-2.js';

var BUGNUMBER = 230216;
var summary = 'check for numerical overflow in regexps in back reference and bounds for {} quantifier';
var actual = '';
var expect = '';
var status = '';

DESCRIPTION = summary;
EXPECTED = 'error';

printBugNumber(BUGNUMBER);
printStatus (summary);

status = inSection(1) + ' check for overflow in quantifier';

actual = 'undefined'; 
expect = 'error';

try
{
  var result = eval('/a{21474836481}/.test("a")');
  actual = 'no exception thrown';
  status += ' result: ' + result;
}
catch(e)
{
  actual = 'error';
}

reportCompare(expect, actual, status);

