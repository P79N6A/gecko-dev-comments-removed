




































var gTestfile = 'regress-338121-03.js';

var BUGNUMBER = 338121;
var summary = 'Issues with JS_ARENA_ALLOCATE_CAST';
var actual = 'No Crash';
var expect = 'No Crash';

printBugNumber(BUGNUMBER);
printStatus (summary);

try
{
  var fe="vv";

  for (i=0; i<24; i++)
    fe += fe;

  var fu=new Function(
    fe, fe, fe, fe, fe, fe, fe, fe, fe, fe, fe, fe, fe, fe, fe, fe, fe, fe, fe,
    fe, fe, fe, fe, fe, fe, fe, fe, fe, fe, fe, fe, fe, fe, fe, fe, fe, fe, fe,
    fe, fe, fe, fe, fe, fe, fe, fe, fe, fe, fe, fe, fe, fe, fe, fe, fe, fe, fe,
    fe, fe, fe,
    "done"
    );



  var fuout = 'fu=' + fu;
}
catch(ex)
{
  expect = 'InternalError: script stack space quota is exhausted';
  actual = ex + '';
  print('Caught ' + ex);
}
print('Done');

reportCompare(expect, actual, summary);
