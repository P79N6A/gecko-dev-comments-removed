




































var gTestfile = '8.6.1-01.js';


var BUGNUMBER = 315436;
var summary = 'In strict mode, setting a read-only property should generate a warning';

printBugNumber(BUGNUMBER);
printStatus (summary);

enterFunc (String (BUGNUMBER));


var actual = '';
var expect = 's.length is read-only';
var status = summary + ': Throw if STRICT and WERROR is enabled';

if (!options().match(/strict/))
{
  options('strict');
}
if (!options().match(/werror/))
{
  options('werror');
}

try
{
  var s = new String ('abc');
  s.length = 0;
}
catch (e)
{
  actual = e.message;
}

reportCompare(expect, actual, status);



actual = 'did not throw';
expect = 'did not throw';
var status = summary + ': Do not throw if STRICT is enabled and WERROR is disabled';


options('werror');

try
{
  s.length = 0;
}
catch (e)
{
  actual = e.message;
}

reportCompare(expect, actual, status);



actual = 'did not throw';
expect = 'did not throw';
var status = summary + ': Do not throw if not in strict mode';


options('strict');

try
{
  s.length = 0;
}
catch (e)
{
  actual = e.message;
}

reportCompare(expect, actual, status);
