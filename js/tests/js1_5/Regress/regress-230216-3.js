





































var gTestfile = 'regress-230216-3.js';

var BUGNUMBER = 230216;
var summary = 'check for numerical overflow in regexps in back reference and bounds for {} quantifier';
var actual = '';
var expect = '';
var status = '';

printBugNumber(BUGNUMBER);
printStatus (summary);

status = inSection(1) + ' /((\3|b)\2(a)x)+/.exec("aaxabxbaxbbx") ';

actual = 'undefined'; 
expect = ['ax', 'ax', '', 'a'] + '';

try
{ 
  actual = /((\3|b)\2(a)x)+/.exec("aaxabxbaxbbx") + '';
}
catch(e)
{
  status += ' Error: ' + e;
}

reportCompare(expect, actual, status);

status = inSection(2) + ' eval(\'/((\3|b)\2(a)x)+/.exec("aaxabxbaxbbx")\' ';

actual = 'undefined'; 
expect = ['ax', 'ax', '', 'a'] + '';

try
{ 
  actual = eval('/((\\3|b)\\2(a)x)+/.exec("aaxabxbaxbbx")') + '';
}
catch(e)
{
  status += ' Error: ' + e;
}

reportCompare(expect, actual, status);
