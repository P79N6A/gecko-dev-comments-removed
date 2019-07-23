




































var gTestfile = 'regress-333728.js';

var BUGNUMBER = 333728;
var summary = 'Throw ReferenceErrors for typeof(...undef)';
var actual = '';
var expect = 'ReferenceError';

printBugNumber(BUGNUMBER);
printStatus (summary);

try
{
  actual = typeof (0, undef);
}
catch(ex)
{
  actual = ex.name;
}
 
reportCompare(expect, actual, summary + ': typeof (0, undef)');

try
{
  actual = typeof (0 || undef);
}
catch(ex)
{
  actual = ex.name;
}
 
reportCompare(expect, actual, summary + ': typeof (0 || undef)');

try
{
  actual = typeof (1 && undef);
}
catch(ex)
{
  actual = ex.name;
}
 
reportCompare(expect, actual, summary + ': typeof (1 && undef)');



























try
{
  actual = typeof (!this ? 0 : undef);
}
catch(ex)
{
  actual = ex.name;
}
 
reportCompare(expect, actual, summary + ': typeof (!this ? 0 : undef)');
