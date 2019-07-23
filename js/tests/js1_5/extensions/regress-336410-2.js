




































var gTestfile = 'regress-336410-2.js';

var BUGNUMBER = 336410;
var summary = 'Integer overflow in array_toSource';
var actual = 'No Crash';
var expect = 'No Crash';

printBugNumber(BUGNUMBER);
printStatus (summary);

expectExitCode(0);
expectExitCode(5);

function createString(n)
{
  var l = n*1024*1024;
  var r = 'r';

  while (r.length < l)
  {
    r = r + r;
  }
  return r;
}

try
{
  var n = 128;
  printStatus('Creating ' + n + 'M length string');
  var r = createString(n);
  printStatus('Done. length = ' + r.length);
  printStatus('Creating array');
  var o=[r, r, r, r, r, r, r, r, r];
  printStatus('object.toSource()');
  var rr = o.toSource();
  printStatus('Done.');
}
catch(ex)
{
  expect = 'InternalError: script stack space quota is exhausted';
  actual = ex + '';
  print(actual);
}

reportCompare(expect, actual, summary);
