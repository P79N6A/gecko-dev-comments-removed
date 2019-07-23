




































var bug = 336409;
var summary = 'Integer overflow in js_obj_toSource';
var actual = 'No Crash';
var expect = 'No Crash';

printBugNumber (bug);
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

var n = 64;
printStatus('Creating ' + n + 'MB string');
var r = createString(n);
printStatus('Done. length = ' + r.length);
printStatus('Creating object');
var o = {f1: r, f2: r, f3: r,f4: r,f5: r, f6: r, f7: r, f8: r,f9: r};
printStatus('object.toSource()');
var rr = o.toSource();
printStatus('Done.');

reportCompare(expect, actual, summary);
