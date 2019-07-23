




































var gTestfile = 'regress-338001.js';

var BUGNUMBER = 338001;
var summary = 'integer overflow in jsfun.c:Function';
var actual = 'No Crash';
var expect = 'No Crash';

printBugNumber(BUGNUMBER);
printStatus (summary);

expectExitCode(0);
expectExitCode(5);

var fe="f";

try
{
  for (i=0; i<25; i++)
    fe += fe;

  var fu=new Function(
    fe, fe, fe, fe, fe, fe, fe, fe, fe, fe, fe, fe, fe, fe, fe, fe, fe, fe,
    fe, fe, fe, fe, fe, fe, fe, fe, fe, fe, fe, fe, fe, fe, fe, fe, fe, fe,
    fe, fe, fe, fe, fe, fe, fe, fe, fe, fe, fe, fe, fe, fe, fe, fe, fe, fe,
    fe, fe, fe, fe, fe, fe, fe, fe, fe, fe,
    "done"
    );
}
catch(ex)
{
  
  expect = 'InternalError: allocation size overflow';
  actual = ex + '';
}
 
print('Done');

reportCompare(expect, actual, summary);
