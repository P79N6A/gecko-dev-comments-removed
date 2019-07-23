




































gTestfile = 'regress-319872.js';


var BUGNUMBER = 319872;
var summary = 'Do not Crash in jsxml.c';
var actual = 'No Crash';
var expect = /(No Crash|InternalError: script stack space quota is exhausted|InternalError: allocation size overflow)/;

printBugNumber(BUGNUMBER);
START(summary);
printStatus ("Expect either no error, out of memory or catchable script stack " + 
             "space quota is exhausted error");
expectExitCode(0);
expectExitCode(3);
expectExitCode(5);

try
{
  var i,m,str;
  str="<a xmlns:v=\"";
  m="";

  for (i=0;i<(1024*1024)/2;i++)
    m += "\n";

  for(i=0;i<500;i++)
    str += m ;

  str += "\">f00k</a>";

  var xx = new XML(str);

  printStatus(xx.toXMLString());
}
catch(ex)
{
  actual = ex + '';
  print(actual);
}
reportMatch(expect, actual, summary);
