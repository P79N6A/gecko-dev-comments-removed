





































var gTestfile = 'regress-230216-1.js';

var BUGNUMBER = 230216;
var summary = 'check for numerical overflow in regexps in back reference and bounds for {} quantifier';
var actual = '';
var expect = '';
var status = '';

printBugNumber(BUGNUMBER);
printStatus (summary);

status = inSection(1) + ' check for overflow in backref';

actual = 'undefined'; 
expect = false;

try
{
  actual = eval('/(a)\21474836481/.test("aa")');
}
catch(e)
{
  status += ' Error: ' + e;
}

reportCompare(expect, actual, status);

status = inSection(2) + ' check for overflow in backref';

actual = 'undefined'; 
expect = false;

try
{ 
  actual = eval('/a\21474836480/.test("")');
}
catch(e)
{
  status += ' Error: ' + e;
}

reportCompare(expect, actual, status);

