





































var BUGNUMBER = 338001;
var summary = 'integer overflow in jsfun.c:Function';
var actual = 'No Crash';
var expect = /No Crash|InternalError: allocation size overflow/;

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
  
  actual = ex + '';
}
 
print('Done: ' + actual);

reportMatch(expect, actual, summary);
